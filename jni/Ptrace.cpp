/*
 * Copyright 2024 DeNA Co., Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#define _LARGEFILE64_SOURCE

#include <assert.h>
#include <dirent.h> // opendir用
#include <errno.h>
#include <fcntl.h>
#include <memory>
#include <stdlib.h>
#include <string.h>
#include <sys/ptrace.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "Ptrace.h"
#include "Utility.h"

// 全スレッドをATTACHしてからwaitする
// 使い方が合っているかどうかは不明
bool Ptrace::Attach() {
  assert(pid_ >= 0);
  ClearCache();
  if (without_ptrace_ || attached_) {
    attached_ = true;
    return true;
  }
  LoadThreadIDs();
  Utility::DebugLog("Start Attach %d", pid_);
  for (auto it = thread_ids_.begin(); it != thread_ids_.end(); it++) {
    if (ptrace(PTRACE_ATTACH, *it, nullptr, nullptr)) {
      Utility::PrintErrnoString("Can't attach pid=%d tid=%d", pid_, *it);
      Detach();
      return false;
    }
    Utility::DebugLog("  Attach %d thread", *it);
  }
  int status = 0;
  if (waitpid(-1, &status, __WALL) == -1) {
    Utility::PrintErrnoString("Fail wait pid=%d", pid_);
    return false;
  }
  attached_ = true;
  Utility::DebugLog("Attach  %d", pid_);
  return true;
}

bool Ptrace::Detach() {
  if (without_ptrace_ || !attached_) {
    attached_ = false;
    return true;
  }
  for (auto it = thread_ids_.begin(); it != thread_ids_.end(); it++) {
    if (ptrace(PTRACE_DETACH, *it, nullptr, nullptr)) {
      Utility::PrintErrnoString("Can't detach pid=%d, tid=%d", pid_, *it);
    }
  }
  attached_ = false;
  Utility::DebugLog("Detach %d", pid_);
  return true;
}

size_t Ptrace::Read(uint8_t *dest, const Range &src) const {
  assert(pid_ >= 0);
  assert(attached_);
  size_t n = src.Size();
  char path[100];
  snprintf(path, 99, "/proc/%d/mem", pid_);

  int fd = open(path, O_RDONLY);
  lseek64(fd, (unsigned long)src.GetStart().to_i(), SEEK_SET);
  size_t ret = read(fd, dest, n);
  close(fd);
  if (ret == -1) {
    Utility::DebugLog("*** Error : Memory Read is failed ***\nAddress: %zx\n%s (errno=%d)\n", src.GetStart().to_i(),
                      strerror(errno), errno);
    return 0;
  } else if (ret != n) {
    Utility::DebugLog("*** Error : Memory Read is failed ***\nExpected length: "
                      "%zu\nRead length: %zu\nAddress: %zx\n",
                      n, ret, src.GetStart().to_i());
  }

  return ret;
}

size_t Ptrace::ReadWithCache(uint8_t *dest, const Range &src, const Range &parent_range) const {
  assert(pid_ >= 0);
  assert(attached_);
  assert(parent_range.IsSuperset(src));
  if (parent_range.GetStart().to_i() != cache_range_.GetStart().to_i() ||
      parent_range.GetEnd().to_i() != cache_range_.GetEnd().to_i()) {
    cache_range_ = parent_range;
    cache_ = std::make_unique<uint8_t[]>(cache_range_.Size());
    Read(cache_.get(), parent_range);
  }

  size_t offset = src.GetStart().to_i() - cache_range_.GetStart().to_i();
  memcpy(dest, cache_.get() + offset, src.Size());

  return src.Size();
}

// size_t Ptrace::ReadByPeekData(uint8_t *dest, const Address &src) const {
//     // if (src.to_i() % sizeof(size_t) != 0) {
//     //     Utility::DebugLog("Alignment is illegal addr=%zx (fix me)",
//     src.to_i());
//     // }
//     assert(pid_ >= 0);
//     assert(attached_);
//     errno = 0;
//     long word = ptrace(PTRACE_PEEKDATA, pid_, src.to_i(), nullptr);
//     if (errno != 0) {
//         Utility::PrintErrnoString("Can't pokedata pid=%d addr=%zx", pid_,
//         src.to_i()); return 0;
//     }
//     memcpy(dest, &word, sizeof(long));
//     return sizeof(long);
// }
//
// size_t Ptrace::ReadByPeekData(uint8_t *dest, const Range &src) const {
//     assert(pid_ >= 0);
//     assert(attached_);
//     size_t n = src.GetEnd().to_i() - src.GetStart().to_i();
//     Address p = src.GetStart();
//     Utility::DebugLog("%zx-%zx (%s)", src.GetStart().to_i(),
//     src.GetEnd().to_i(), src.GetComment().c_str()); for (size_t i = 0; i < n;
//     i += sizeof(long)) {
//         size_t s = std::min(sizeof(long), n - i);
//         if (s != sizeof(long)) {
//             uint8_t temp[sizeof(long)];
//             if (!ReadByPeekData(temp, p)) { return i; }
//             for (size_t j = 0; j < s; j++) { dest[j] = temp[j]; }
//         } else {
//             if (!ReadByPeekData(dest, p)) { return i; }
//         }
//         dest += sizeof(long);
//         p.Add(sizeof(long));
//     }
//     return n;
// }

size_t Ptrace::WriteByPokeData(const Address &dest, long value) const {
  assert(pid_ >= 0);
  assert(attached_);
  assert(!without_ptrace_);
  // ptraceのPOKEDATAの第4引数はlongの値が入ったvoid*型（アドレスではない）
  if (ptrace(PTRACE_POKEDATA, pid_, (void *)dest.to_i(), (void *)value)) {
    Utility::PrintErrnoString("Can't pokedata pid=%d addr=%zx value=%zx", pid_, dest.to_i(), value);
    return 0;
  }
  return sizeof(long);
}

size_t Ptrace::WriteByPokeData(const Range &dest, const uint8_t *src, bool freeze_request) const {
  Utility::DebugLog("Using Poke");
  assert(pid_ >= 0);
  assert(attached_);
  assert(!without_ptrace_);
  if (freeze_request) {
    Utility::DebugLog("*** Error : Can't use freeze mode with ptrace");
    return 0;
  }
  size_t n = dest.GetEnd().to_i() - dest.GetStart().to_i();
  Address p = dest.GetStart();
  for (size_t i = 0; i < n; i += sizeof(long)) {
    size_t s = std::min(sizeof(long), n - i);
    uint8_t temp[sizeof(long)];
    memcpy(temp, src, s);
    if (s != sizeof(long)) {
      // longのサイズでしか書き込めないので後ろの部分をReadで読み込んでメモリ上にあるデータと同じにする
      Range rest = Range(dest.GetEnd().to_i(), dest.GetEnd().to_i() + sizeof(long) - s, dest.GetComment());
      if (!Read(temp + s, rest)) {
        return i;
      }
      if (!WriteByPokeData(p, *(long *)temp)) {
        return i;
      }
    } else {
      if (!WriteByPokeData(p, *(long *)temp)) {
        return i;
      }
    }
    src += sizeof(long);
    p.Add(sizeof(long));
  }
  return dest.Size();
}

size_t Ptrace::Write(const Range &dest, const uint8_t *src, bool freeze_request) const {
  if (!without_ptrace_) {
    return WriteByPokeData(dest, src, freeze_request);
  }
  assert(pid_ >= 0);
  assert(attached_ || freeze_request);
  size_t n = dest.GetEnd().to_i() - dest.GetStart().to_i();

  char path[100];
  snprintf(path, 99, "/proc/%d/mem", pid_);

  int fd = open(path, O_WRONLY);
  lseek64(fd, (unsigned long)dest.GetStart().to_i(), SEEK_SET);
  size_t ret = write(fd, src, n);
  close(fd);
  if (ret == -1) {
    Utility::DebugLog("*** Error : Memory Write is failed ***\nAddress: %zx\n%s (errno=%d)\n", dest.GetStart().to_i(),
                      strerror(errno), errno);
    return 0;
  } else if (ret != n) {
    Utility::DebugLog("*** Error : Memory Write is failed ***\nExpected "
                      "length: %zu\nWrite length: %zu\nAddress: %zx\n",
                      n, ret, dest.GetStart().to_i());
  }

  return ret;

  // Address p = dest.GetStart();
  // for (size_t i = 0; i < n; i += sizeof(long)) {
  //     size_t s = std::min(sizeof(long), n - i);
  //     if (s != sizeof(long)) {
  //         uint8_t temp[sizeof(long)];
  //         if (!Read(temp, Range(p.to_i(), p.to_i() + sizeof(long),
  //         p.GetComment()))) { return i; } for (size_t j = 0; j < s; j++) {
  //         temp[j] = src[j]; } if (!Write(p, temp)) { return i; }
  //     } else {
  //         if (!Write(p, src)) { return i; }
  //     }
  //     p.Add(s);
  //     src += s;
  // }
  // return n;
}

void Ptrace::Dump(const Range &src) const {
  assert(pid_ >= 0);
  assert(attached_);
  size_t n = src.GetEnd().to_i() - src.GetStart().to_i();
  if (n == 0) {
    Utility::DebugLog("Dump 0-0 length:0(NaN%%)");
    return;
  }
  const std::unique_ptr<uint8_t[]> temp_p = std::make_unique<uint8_t[]>(n);
  size_t l = Read(temp_p.get(), src);
  Utility::DebugLog("%s",
                    Utility::HexDump(src.GetStart().to_i(), src.GetComment().c_str(), temp_p.get(), l, 2).c_str());
}

// /proc/[pid]/task/* からThread IDを引っ張ってくる
void Ptrace::LoadThreadIDs() {
  assert(pid_ != -1);
  char path[100];
  snprintf(path, 99, "/proc/%d/task", pid_);

  DIR *dp = opendir(path);
  if (dp == nullptr) {
    fprintf(stderr, "Can't open %s\n", path);
    return;
  }
  thread_ids_.clear();
  struct dirent *directory = nullptr;
  while ((directory = readdir(dp)) != nullptr) {
    int tid = atoi(directory->d_name);
    if (tid != 0) {
      thread_ids_.insert(tid);
    }
  }
  closedir(dp);
}

void Ptrace::ClearCache() {
  cache_range_ = Range();
  cache_.reset();
}
