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
#include <algorithm>
#include <assert.h>
#include <map>
#include <memory>
#include <sstream>
#include <string.h>
#include <time.h>
#include <vector>

#include "Converter.h"
#include "Patcher.h"
#include "Snapshot.h"
#include "Utility.h"

std::string Patcher::GetModeString(Mode mode) {
  std::map<Mode, std::string> temp = {
      {Mode::NOP, "nop"},
      {Mode::LOOKUP, "lookup"},
      {Mode::FILTER, "filter"},
      {Mode::CHANGE, "change"},
  };
  assert(temp.count(mode));
  return temp[mode];
}
Patcher::Mode Patcher::GetMode(const std::string &str) {
  std::map<std::string, Mode> temp = {
      {"nop", Mode::NOP},  {"lookup", Mode::LOOKUP}, {"filter", Mode::FILTER}, {"change", Mode::CHANGE},
      {"l", Mode::LOOKUP}, {"f", Mode::FILTER},      {"c", Mode::CHANGE},
  };
  assert(temp.count(str));
  return temp[str];
}

//================================================================================
// Command
//================================================================================

bool Patcher::Attach(const std::string &command, std::stringstream &sin) { return ptrace_->Attach(); }

bool Patcher::Detach(const std::string &command, std::stringstream &sin) { return ptrace_->Detach(); }

bool Patcher::Clear(const std::string &command, std::stringstream &sin) {
  addr_set_.clear();
  return true;
}

bool Patcher::Process(const std::string &command, std::stringstream &sin) {
  std::string type, str;
  if (!(sin >> type >> str)) {
    return false;
  }
  ChangeString change_str;
  if (!change_str.Init(type, str)) {
    Utility::DebugLog("%s is wrong type", type.c_str());
    return false;
  }
  if (!Process(GetMode(command), change_str)) {
    return false;
  }
  Utility::DebugLog("Please Input Command");
  return true;
}

bool Patcher::PairFilter(const std::string &command, std::stringstream &sin) {
  std::string type, str;
  int store_size;
  if (!(sin >> type >> str >> store_size)) {
    return false;
  }

  std::vector<TargetAddress> prev_addr_set = addr_set_;
  ChangeString change_str;
  if (!change_str.Init(type, str)) {
    Utility::DebugLog("%s is wrong type", type.c_str());
    return false;
  }
  if (!Process(Mode::LOOKUP, change_str)) {
    return false;
  }

  std::vector<std::pair<size_t, TargetAddress>> addr_dists;
  sort(prev_addr_set.begin(), prev_addr_set.end());
  sort(addr_set_.begin(), addr_set_.end());
  {
    // 尺取りメソッドでアドレス間の一番近い距離を求める
    auto it2 = addr_set_.begin();
    for (auto it1 = prev_addr_set.begin(); it1 != prev_addr_set.end(); it1++) {
      size_t dist = 1e+9;
      while (true) {
        auto nit2 = it2 + 1;
        if (nit2 == addr_set_.end()) {
          break;
        }
        size_t ndist = it1->GetAddress().Dist(nit2->GetAddress());
        if (ndist >= dist) {
          break;
        }
        dist = ndist;
        it2 = nit2;
      }
      addr_dists.emplace_back(dist, *it1);
    }
  }
  std::sort(addr_dists.begin(), addr_dists.end());

  addr_set_.clear();
  const int MAX_OUTPUT_SIZE = store_size + 10;
  int output_cnt = 0;
  for (auto it = addr_dists.begin(); it != addr_dists.end() && output_cnt < MAX_OUTPUT_SIZE; it++, output_cnt++) {
    size_t dist = it->first;
    const TargetAddress &addr = it->second;
    if ((int)addr_set_.size() < store_size) {
      addr_set_.emplace_back(addr);
    }
    Utility::DebugLog("Dist: %zd, Address: %zx", dist, addr.GetAddress().to_i());
  }
  Utility::DebugLog("Result Address: %d", (int)addr_set_.size());

  Utility::DebugLog("");
  Utility::DebugLog("Please Input Command");
  return true;
}

bool Patcher::Replace(const std::string &command, std::stringstream &sin) {
  std::string hex_start, string_type, after;
  size_t start;
  if (!(sin >> hex_start >> string_type >> after) || sscanf(hex_start.c_str(), "%zx", &start) != 1) {
    return false;
  }
  // TODO
  ChangeString change_str;
  if (!change_str.Init(string_type, after)) {
    Utility::DebugLog("%s %s is not same length or wrong type", string_type.c_str(), after.c_str());
    return false;
  }

  if (!ptrace_->Attach() || !CreateRangeSet()) {
    return false;
  }
  TargetAddress address(Range::Fit(range_set_, Address(start)), change_str);
  if (address.GetAddress().to_i() == 0) {
    Utility::DebugLog("Target Address is over the memory range");
    return false;
  }
  Replace(address, change_str);
  return true;
}

bool Patcher::Freeze(const std::string &command, std::stringstream &sin) {
  std::string hex_start, string_type, after;
  size_t start;
  if (!(sin >> hex_start >> string_type >> after) || sscanf(hex_start.c_str(), "%zx", &start) != 1) {
    return false;
  }
  ChangeString change_str;
  if (!change_str.Init(string_type, after)) {
    Utility::DebugLog("%s %s is not same length or wrong type", string_type.c_str(), after.c_str());
    return false;
  }

  if (!ptrace_->Attach() || !CreateRangeSet()) {
    return false;
  }
  TargetAddress address(Range::Fit(range_set_, Address(start)), change_str);
  if (address.GetAddress().to_i() == 0) {
    Utility::DebugLog("Target Address is over the memory range");
    return false;
  }
  freeze_set_.push_back(std::make_unique<FreezeThread>(ptrace_, address));
  freeze_set_.back()->Start();
  return true;
}

bool Patcher::FreezeTerminate(const std::string &command, std::stringstream &sin) {
  FreezeTerminate();
  return true;
}

bool Patcher::Diff(const std::string &command, std::stringstream &sin) {
  std::string mode_str;
  sin >> mode_str;

  const auto start_time = std::chrono::steady_clock::now();
  Utility::DebugLog("Starting memory patching... (mode: diff)");
  enum class DiffMode {
    NOP,
    START,
    UPPER,
    LOWER,
    SAME,
    CHANGE,
    END,
  };
  std::map<std::string, DiffMode> temp = {
      {"nop", DiffMode::NOP},   {"start", DiffMode::START},   {"upper", DiffMode::UPPER}, {"lower", DiffMode::LOWER},
      {"same", DiffMode::SAME}, {"change", DiffMode::CHANGE}, {"end", DiffMode::END},
  };
  DiffMode mode = temp[mode_str];
  if (mode == DiffMode::LOWER || mode == DiffMode::UPPER || mode == DiffMode::SAME || mode == DiffMode::CHANGE) {
    range_set_.clear();
    if (!ptrace_->Attach() || !CreateRangeSet()) {
      return false;
    }

    if (snapshot_) {
      addr_set_.clear();
      for (SnappedRange sr : *snapshot_) {
        Range range = Range::Fit(range_set_, sr.range());
        if (range.GetStart().to_i() == 0)
          continue;

        size_t start = range.GetStart().to_i();
        // size_t end = range.GetEnd().to_i();
        size_t n = range.Size();

        const std::unique_ptr<uint8_t[]> old_memory = sr.data();
        const std::unique_ptr<uint8_t[]> new_memory = std::make_unique<uint8_t[]>(n);
        ptrace_->Read(new_memory.get(), range);

        for (size_t i = 0; i < n; i += 4) {
          int old_value = *(int *)(old_memory.get() + i);
          int new_value = *(int *)(new_memory.get() + i);
          // ChangeStringを中で作ってるのは数倍くらい速度が変わるため
          if (mode == DiffMode::UPPER && old_value < new_value) {
            ChangeString change_str(new_value);
            addr_set_.emplace_back(Address(start + i), change_str);
          } else if (mode == DiffMode::LOWER && old_value > new_value) {
            ChangeString change_str(new_value);
            addr_set_.emplace_back(Address(start + i), change_str);
          } else if (mode == DiffMode::SAME && old_value == new_value) {
            ChangeString change_str(new_value);
            addr_set_.emplace_back(Address(start + i), change_str);
          } else if (mode == DiffMode::CHANGE && old_value != new_value) {
            ChangeString change_str(new_value);
            addr_set_.emplace_back(Address(start + i), change_str);
          } else {
            // nop
          }
        }
      }
      snapshot_.reset();
    } else {
      const std::unique_ptr<uint8_t[]> temp_p = std::make_unique<uint8_t[]>(4);
      int cnt = 0;
      auto parent_range_it = range_set_.begin();
      for (auto it = addr_set_.begin(); it != addr_set_.end(); it++) {
        // TODO 関数化してFilterと合わせる
        ChangeString change_str = it->GetChangeString();
        size_t start = it->GetAddress().to_i();
        size_t end = it->GetAddress().to_i() + change_str.Size();
        if (end - start != 4) {
          Utility::DebugLog("Target Rage is not 4: %zx-%zx (%d)", start, end, end - start);
          continue;
        }

        while (parent_range_it != range_set_.end() && parent_range_it->GetEnd().to_i() < start) {
          parent_range_it++;
        }
        if (parent_range_it == range_set_.end() || !parent_range_it->IsSuperset(Range(start, end, ""))) {
          continue;
        }
        size_t s = -1;
        if (addr_set_.size() < 10000) {
          s = ptrace_->Read(temp_p.get(), Range(start, end, parent_range_it->GetComment()));
        } else {
          // open, readのsyscallが重いので、個数が多い場合はキャッシュ付きでやる
          s = ptrace_->ReadWithCache(temp_p.get(), Range(start, end, parent_range_it->GetComment()), *parent_range_it);
        }
        if (s != end - start) {
          continue;
        }

        int old_value = *(int *)change_str.GetRawValue().data();
        int new_value = *(int *)temp_p.get();
        TargetAddress target_address(it->GetAddress(), ChangeString(new_value));
        if (mode == DiffMode::UPPER && old_value < new_value) {
          addr_set_[cnt++] = target_address;
        } else if (mode == DiffMode::LOWER && old_value > new_value) {
          addr_set_[cnt++] = target_address;
        } else if (mode == DiffMode::SAME && old_value == new_value) {
          addr_set_[cnt++] = target_address;
        } else if (mode == DiffMode::CHANGE && old_value != new_value) {
          addr_set_[cnt++] = target_address;
        } else {
          // nop
        }
      }
      addr_set_.resize(cnt);
    }
    Utility::DebugLog("Found! %zd address", addr_set_.size());
  } else if (mode == DiffMode::START) {
    snapshot_ = std::make_unique<Snapshot>();
    if (!ptrace_->Attach() || !CreateRangeSet()) {
      return false;
    }

    for (RangeSet::const_iterator it = range_set_.begin(); it != range_set_.end(); ++it) {
      size_t n = it->Size();
      const std::unique_ptr<uint8_t[]> temp_p = std::make_unique<uint8_t[]>(n);
      ptrace_->Read(temp_p.get(), *it);
      snapshot_->push_back(*it, temp_p.get());
    }
    Utility::DebugLog("snapshot created!");
  } else if (mode == DiffMode::END) {
    Utility::DebugLog("snapshot done! Use change command.");
    snapshot_.reset();
  } else {
    Utility::DebugLog("usage: diff [start|end|upper|lower|same|change]");
  }

  const auto end_time = std::chrono::steady_clock::now();
  double duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
  Utility::DebugLog("Process Time: %.0lf ms", duration);

  return true;
}

bool Patcher::Result(const std::string &command, std::stringstream &sin) { return OutputResult(stdout); }
bool Patcher::Scope(const std::string &command, std::stringstream &sin) {
  std::string scope;
  if (!(sin >> scope)) {
    range_scope_ = "";
  } else {
    range_scope_ = scope;
  }
  Utility::DebugLog("Now scope is '%s'", range_scope_.c_str());
  return true;
}
bool Patcher::Save(const std::string &command, std::stringstream &sin) {
  std::string filename;
  if (!(sin >> filename)) {
    filename = "/sdcard/mempatch_state.txt";
  }
  FILE *fp = fopen(filename.c_str(), "wb");
  if (fp == nullptr) {
    return false;
  }
  Serialize(fp);
  fclose(fp);
  return true;
}
bool Patcher::Load(const std::string &command, std::stringstream &sin) {
  std::string filename;
  if (!(sin >> filename)) {
    filename = "/sdcard/mempatch_state.txt";
  }
  FILE *fp = fopen(filename.c_str(), "rb");
  if (fp == nullptr) {
    return false;
  }
  DeSerialize(fp);
  fclose(fp);
  return true;
}

bool Patcher::Dump(const std::string &command, std::stringstream &sin) {
  std::string hex_start, hex_len;
  size_t start, len;
  if (!(sin >> hex_start >> hex_len) || sscanf(hex_start.c_str(), "%zx", &start) != 1 ||
      sscanf(hex_len.c_str(), "%zx", &len) != 1 || start > start + len) {
    return false;
  }
  if (ptrace_->Attach() && CreateRangeSet()) {
    ptrace_->Dump(Range::Fit(range_set_, Range(start, start + len, "")));
  } else {
    Utility::DebugLog("Failed Dumping");
  }
  return true;
}

bool Patcher::DumpAll(const std::string &command, std::stringstream &sin) {
  std::string filename;
  if (!(sin >> filename)) {
    filename = "/sdcard/mempatch_dump.dat";
  }
  return DumpAll(filename);
}

bool Patcher::Help(const std::string &command, std::stringstream &sin) {
  PrintCommandUsage();
  return true;
}

bool Patcher::Exit(const std::string &command, std::stringstream &sin) {
  Exit();
  return true;
}

//================================================================================
// misc
//================================================================================

bool Patcher::SaveResult(const std::string &filename) const {
  FILE *fp = nullptr;
  fp = fopen(filename.c_str(), "w");
  if (fp == nullptr) {
    Utility::DebugLog("result file '%s' can't open", filename.c_str());
    return false;
  }
  bool ret = OutputResult(fp);
  fclose(fp);
  return ret;
}

bool Patcher::OutputResult(FILE *fp) const {
  fprintf(fp, "Last PorcessTime %d\n", last_process_time_);

  fprintf(fp, "Target Address : %d\n", (int)range_set_.size());
  for (auto it = range_set_.begin(); it != range_set_.end(); it++) {
    if (sizeof(size_t) == 4) {
      fprintf(fp, "    %08zx-%08zx (%s, %zd Byte)\n", it->GetStart().to_i(), it->GetEnd().to_i(),
              it->GetComment().c_str(), it->Size());
    } else if (sizeof(size_t) == 8) {
      fprintf(fp, "    %016zx-%016zx (%s, %zd Byte)\n", it->GetStart().to_i(), it->GetEnd().to_i(),
              it->GetComment().c_str(), it->Size());
    } else {
      assert(false);
    }
  }

  fprintf(fp, "Address Set : %d\n", (int)addr_set_.size());
  for (auto it = addr_set_.begin(); it != addr_set_.end(); it++) {
    if (sizeof(size_t) == 4) {
      fprintf(fp, "    %08zx : ", it->GetAddress().to_i());
    } else {
      fprintf(fp, "    %016zx : ", it->GetAddress().to_i());
    }
    fprintf(fp, "%s", it->GetChangeString().GetValue().c_str());
    fprintf(fp, " (%s)", it->GetChangeString().GetTypeString().c_str());
    fprintf(fp, " (%s)\n", it->GetAddress().GetComment(range_set_).c_str());
  }

  return true;
}

void Patcher::FreezeTerminate() {
  for (auto &freeze_ : freeze_set_) {
    freeze_->Terminate();
  }
  freeze_set_.clear();
}
void Patcher::Exit() {
  FreezeTerminate();
  ptrace_->Detach();
}

//================================================================================
// private
//================================================================================

/**
 * /proc/[pid]/maps からマッピングされている読み書き可能なメモリ領域を列挙する
 *
 * @param rset 読み書き可能なメモリ領域
 */
bool Patcher::CreateRangeSet() {
  assert(ptrace_->IsAttached());
  FILE *fp = nullptr;
  char mmap_path[64];
  char mmap_line[4096];
  const std::vector<std::string> ignore_list = {
      // 対象外のディレクトリ
      "/system/lib/",
      "/lib/x86_64-linux-gnu/",
      "/usr/lib/",
  };

  sprintf(mmap_path, "/proc/%d/maps", ptrace_->GetPid());
  fp = fopen(mmap_path, "r");
  if (fp == nullptr) {
    Utility::DebugLog("process maps file '%s' can't be opend", mmap_path);
    return false;
  }

  RangeSet prev_rset = range_set_;
  range_set_.clear();

  for (int i = 0; fgets(mmap_line, sizeof(mmap_line), fp) != nullptr; i++) {
    std::stringstream sin(mmap_line);
    std::string address, permission, offset, dev, inode, pathname;
    pathname = "";
    sin >> address >> permission >> offset >> dev >> inode >> pathname;
    // 読み込み権限・書き込み権限があり、sharedでもファイルでない物のみ対象
    for (auto it = ignore_list.begin(); it != ignore_list.end(); it++) {
      if (pathname.find(*it) != std::string::npos) {
        goto next;
      }
    }
    if (!range_scope_.empty() && pathname.find(range_scope_) == std::string::npos) {
      goto next;
    }
    if (permission[0] == 'r' && permission[1] == 'w' && permission[3] != 's') {
      size_t start, end;
      sscanf(mmap_line, "%zx-%zx", &start, &end);
      range_set_.insert(Range(start, end, pathname));
    }
  next:;
  }
  fclose(fp);
  return true;
}

/**
 * LookUp, Filter, Changeのどれかを行い、計算結果のサマリーを表示する
 */
bool Patcher::Process(const Mode mode, const ChangeString &change_str) {
  bool ret = true;

  const auto start_time = std::chrono::steady_clock::now();
  Utility::DebugLog("Starting memory patching... (mode: %s)", GetModeString(mode).c_str());
  if (mode == Patcher::Mode::NOP) {
  } else if (mode == Patcher::Mode::LOOKUP) {
    ret &= LookUp(change_str);
  } else if (mode == Patcher::Mode::FILTER) {
    ret &= Filter(change_str);
  } else if (mode == Patcher::Mode::CHANGE) {
    ret &= ReplaceAll(change_str);
  } else {
    ret = false;
    Utility::DebugLog("mode is invalid");
  }
  last_process_time_ = time(nullptr);
  const auto end_time = std::chrono::steady_clock::now();
  double duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();

  if (ret) {
    Utility::DebugLog("Process Time: %.0lf ms", duration);
    Utility::DebugLog("Memory: %.2lf MB", (double)GetMemorySize() / 1024.0 / 1024.0);
    Utility::DebugLog("Range Size: %d", GetRangeSetSize());
    Utility::DebugLog("Found Address: %d", GetTargetAddressSetSize());
  }
  Utility::DebugLog("");

  return ret;
}

/**
 * change_strで指定された文字列を含むメモリアドレスを列挙する
 */
bool Patcher::LookUp(const ChangeString &change_str) {
  if (!ptrace_->Attach() || !CreateRangeSet()) {
    return false;
  }
  addr_set_.clear();

  for (RangeSet::const_iterator it = range_set_.begin(); it != range_set_.end(); ++it) {
    size_t start = it->GetStart().to_i();
    size_t end = it->GetEnd().to_i();
    size_t n = end - start;
    const std::unique_ptr<uint8_t[]> temp_p = std::make_unique<uint8_t[]>(n);
    ptrace_->Read(temp_p.get(), *it);
    size_t chstring_len = change_str.Size();
    if (n < chstring_len) {
      continue;
    }
    const uint8_t *raw_before = change_str.GetRawValue().data();
    const uint8_t *str = temp_p.get();

    std::vector<size_t> find_index;
    if (change_str.GetType() == Converter::Type::FLOAT_FUZZY_LITTLE_ENDIAN) {
      find_index = Utility::StrstrByFloatFuzzyLookup(str, raw_before, n, chstring_len);
    } else {
      find_index = Utility::StrstrByRollingHash(str, raw_before, n, chstring_len);
    }

    for (auto it = find_index.begin(); it != find_index.end(); it++) {
      addr_set_.emplace_back(Address(*it + start), change_str);
    }
  }

  return true;
}

/**
 * addrsetのアドレスの値でchange_strに入ってないアドレスを除去する
 */
bool Patcher::Filter(const ChangeString &change_str) {
  if (!ptrace_->Attach() || !CreateRangeSet()) {
    return false;
  }
  auto parent_range_it = range_set_.begin();
  int cnt = 0;
  for (auto it = addr_set_.begin(); it != addr_set_.end(); it++) {
    size_t start = it->GetAddress().to_i();
    size_t end = it->GetAddress().to_i() + change_str.Size();
    // キャッシュ付きでやるために元々あったRangeを探す
    while (parent_range_it != range_set_.end() && parent_range_it->GetEnd().to_i() < start) {
      parent_range_it++;
    }
    if (parent_range_it == range_set_.end() || !parent_range_it->IsSuperset(Range(start, end, ""))) {
      continue;
    }

    const std::unique_ptr<uint8_t[]> temp_p = std::make_unique<uint8_t[]>(change_str.Size());
    size_t s = -1;
    if (addr_set_.size() < 10000) {
      s = ptrace_->Read(temp_p.get(), Range(start, end, parent_range_it->GetComment()));
    } else {
      // open, readのsyscallが重いので、個数が多い場合はキャッシュ付きでやる
      s = ptrace_->ReadWithCache(temp_p.get(), Range(start, end, parent_range_it->GetComment()), *parent_range_it);
    }
    if (change_str.GetType() == Converter::Type::FLOAT_FUZZY_LITTLE_ENDIAN) {
      float min = 0.0f;
      memcpy((char *)&min, change_str.GetRawValue().data(), 4);
      float max = min + 1.05f;
      min = min - 0.55f; // 四捨五入のケース対応
      float v = 0.0f;
      memcpy((uint8_t *)&v, temp_p.get(), 4);

      if (v >= min && v <= max) {
        if (cnt < 20)
          Utility::DebugLog("From:%f, To:%f, Value:%f", min, max, v);
        if (cnt == 20)
          Utility::DebugLog(" and more...");
        addr_set_[cnt++] = TargetAddress(it->GetAddress(), change_str);
      }
    } else {
      if (s == change_str.Size() && memcmp(temp_p.get(), change_str.GetRawValue().data(), change_str.Size()) == 0) {
        addr_set_[cnt++] = TargetAddress(it->GetAddress(), change_str);
      }
    }
  }
  addr_set_.resize(cnt);
  return true;
}

/**
 * addrsetのアドレスの値を置換する
 */
bool Patcher::ReplaceAll(const ChangeString &change_str) {
  if (!ptrace_->Attach() || !CreateRangeSet()) {
    return false;
  }
  size_t cnt = 0;
  for (auto it = addr_set_.begin(); it != addr_set_.end(); it++) {
    if (Replace(*it, change_str)) {
      cnt++;
    }
  }
  Utility::DebugLog("Replace Count: %d / %d", (int)cnt, (int)addr_set_.size());
  return cnt == addr_set_.size();
}

/**
 * 一つのアドレスの内容をcahnge_strで書き換える
 * 中身の整合性のチェックとかはしない
 */
bool Patcher::Replace(const TargetAddress &target_address, const ChangeString &change_str) {
  if (!ptrace_->Attach() || !CreateRangeSet()) {
    return false;
  }
  const Address &address = target_address.GetAddress();
  const size_t start = address.to_i();
  const size_t end = address.to_i() + change_str.Size();
  const std::string &comment = address.GetComment(range_set_);
  const size_t n = end - start;
  const std::unique_ptr<uint8_t[]> temp_p = std::make_unique<uint8_t[]>(n);

  // Debug Log
  ptrace_->Read(temp_p.get(), Range(start, end, comment));
  std::vector<uint8_t> byte = Converter::RawByteToByte(temp_p.get(), n);
  Utility::DebugLog("Change: %s(%s) -> %s(%s) (%s)", Converter::ByteToHex(byte).c_str(),
                    Converter::GetString(change_str.GetType(), byte).c_str(), change_str.GetHexValue().c_str(),
                    change_str.GetValue().c_str(), comment.c_str());

  // Replace
  ptrace_->Write(Range(start, end, comment), change_str.GetRawValue().data(), false);

  // Debug Log & Error Check
  ptrace_->Dump(Range::Fit(range_set_, Range(start - 16, end + 16, comment)));
  ptrace_->Read(temp_p.get(), Range(start, end, comment));
  if (memcmp(temp_p.get(), change_str.GetRawValue().data(), n) != 0) {
    // 指定した値に書き換わってなかった場合はエラーを出力
    Utility::DebugLog("*** Error ***\n*** Replace is failed!!! ***\n*** Please "
                      "Change a device ***\n\n");
    return false;
  }
  return true;
}

bool Patcher::DumpAll(const std::string &filename) {
  if (ptrace_->Attach() && CreateRangeSet()) {
    FILE *fp = fopen(filename.c_str(), "wb");
    if (fp == nullptr) {
      return false;
    }
    fprintf(fp, "_%d", ptrace_->GetPid());
    Utility::SetSerialize(fp, range_set_);
    for (RangeSet::const_iterator it = range_set_.begin(); it != range_set_.end(); ++it) {
      DumpRange(fp, *it);
    }
    fclose(fp);
  } else {
    Utility::DebugLog("Failed Dumping");
    return false;
  }
  return true;
}
bool Patcher::DumpRange(FILE *fp, const Range &range) {
  size_t n = range.Size();
  const std::unique_ptr<uint8_t[]> temp_p = std::make_unique<uint8_t[]>(n);
  ptrace_->Read(temp_p.get(), range);
  for (size_t i = 0; i < n; i++) {
    fprintf(fp, "%c", temp_p[i]);
  }
  return true;
}
std::unique_ptr<uint8_t[]> Patcher::LoadDumpRange(FILE *fp, const Range &range) {
  size_t n = range.Size();
  std::unique_ptr<uint8_t[]> ret = std::make_unique<uint8_t[]>(n);
  for (size_t i = 0; i < n; i++) {
    fscanf(fp, "%c", &ret[i]);
  }
  return ret;
}

void Patcher::Serialize(FILE *fp) const {
  fprintf(fp, "_%d", ptrace_->GetPid());
  fprintf(fp, "_%d", last_process_time_);
  Utility::SetSerialize(fp, range_set_);
  Utility::VectorSerialize(fp, addr_set_);
  fprintf(stdout, "Success\n");
}
void Patcher::DeSerialize(FILE *fp) {
  int pid;
  if (fscanf(fp, "_%d", &pid) != 1 || pid != ptrace_->GetPid()) {
    fprintf(stderr, "Error: Process ID is different\n");
    return;
  }
  fscanf(fp, "_%d", &last_process_time_);
  range_set_ = Utility::SetDeSerialize<class Range>(fp);
  addr_set_ = Utility::VectorDeSerialize<class TargetAddress>(fp);
  fprintf(stdout, "Success\n");
}
