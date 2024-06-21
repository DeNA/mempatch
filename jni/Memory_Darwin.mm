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

#include "Memory.h"
#include "Utility.h"
#include <assert.h>
#include <dirent.h> // opendir用
#include <dlfcn.h>
#include <errno.h>
#include <fcntl.h>
#include <fstream>
#include <mach-o/dyld_images.h>
#include <mach/mach.h>
#include <mach/vm_map.h>
#include <mach/vm_region.h>
#include <memory>
#include <sstream>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

extern "C" kern_return_t mach_vm_read_overwrite(vm_map_t, mach_vm_address_t, mach_vm_size_t, mach_vm_address_t,
                                                mach_vm_size_t *);

extern "C" kern_return_t mach_vm_write(vm_map_t, mach_vm_address_t, vm_offset_t, mach_msg_type_number_t);

bool Memory::Attach() {
  assert(pid_ >= 0);
  ClearCache();
  attached_ = true;
  return true;
}

bool Memory::Detach() {
  attached_ = false;
  return true;
}

size_t Memory::Read(uint8_t *dest, const Range &src) const {
  assert(pid_ >= 0);
  assert(attached_);
  size_t n = src.Size();
  kern_return_t kr;
  mach_port_t task;

  kr = task_for_pid(mach_task_self(), pid_, &task);
  if (kr != KERN_SUCCESS) {
    return false;
  }
  mach_vm_size_t out_size;
  kr = mach_vm_read_overwrite(task, src.GetStart().to_i(), n, (mach_vm_size_t)dest, &out_size);
  if (kr != KERN_SUCCESS) {
    Utility::DebugLog("*** Error : Memory Read is failed ***\nAddress: %zx\n%s (errno=%d)\n", src.GetStart().to_i(),
                      strerror(errno), errno);
    return 0;
  } else if (out_size != n) {
    Utility::DebugLog("*** Error : Memory Read is failed ***\nExpected length: "
                      "%zu\nRead length: %zu\nAddress: %zx\n",
                      n, out_size, src.GetStart().to_i());
  }

  return out_size;
}

size_t Memory::ReadWithCache(uint8_t *dest, const Range &src, const Range &parent_range) const {
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

size_t Memory::WriteByPokeData(const Address &dest, long value) const { return 0; }

size_t Memory::WriteByPokeData(const Range &dest, const uint8_t *src, bool freeze_request) const { return 0; }

size_t Memory::Write(const Range &dest, const uint8_t *src, bool freeze_request) const {
  assert(pid_ >= 0);
  assert(attached_ || freeze_request);
  mach_port_t task;
  kern_return_t kr;
  size_t n = dest.GetEnd().to_i() - dest.GetStart().to_i();

  kr = task_for_pid(mach_task_self(), pid_, &task);
  if (kr != KERN_SUCCESS) {
    return false;
  }
  kr = mach_vm_write(task, dest.GetStart().to_i(), (vm_offset_t)src, n);
  if (kr != KERN_SUCCESS) {
    Utility::DebugLog("*** Error : Memory Write is failed ***\nAddress: %zx\n%s (errno=%d)\n", dest.GetStart().to_i(),
                      strerror(errno), errno);
    return 0;
  }

  return n;
}

void Memory::Dump(const Range &src) const {
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

void Memory::LoadThreadIDs() {}

void Memory::ClearCache() {
  cache_range_ = Range();
  cache_.reset();
}

bool Memory::GenerateMaps(std::stringstream &ss) {
  assert(pid_ >= 0);
  mach_port_t task = mach_task_self();
  vm_address_t address = 0;
  vm_size_t size = 0;
  kern_return_t kr;
  natural_t depth = 0;

  while (true) {
    vm_region_submap_info_64 info;
    mach_msg_type_number_t info_count = VM_REGION_SUBMAP_INFO_COUNT_64;

    // メモリリージョン情報を取得
    kr = vm_region_recurse_64(task, &address, &size, &depth, (vm_region_info_64_t)&info, &info_count);
    if (kr != KERN_SUCCESS) {
      if (kr == KERN_INVALID_ADDRESS) {
        break; // アドレス空間の終端に到達した
      }
      return false; // エラーが発生した場合はfalseを返す
    }

    // /proc/pid/maps形式で出力をフォーマット
    ss << std::hex << address << "-" << (address + size) << " " << ((info.protection & VM_PROT_READ) ? 'r' : '-')
       << ((info.protection & VM_PROT_WRITE) ? 'w' : '-') << ((info.protection & VM_PROT_EXECUTE) ? 'x' : '-') << " \n";

    if (info.is_submap) {
      depth += 1; // サブマップの場合はdepthを増加
    } else {
      address += size; // 次のリージョンに移動
    }
  }

  return true;
}