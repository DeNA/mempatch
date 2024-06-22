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
#include <errno.h>
#include <fcntl.h>
#include <memory>
#include <windows.h> // psapi.hより必ず上に配置
#include <psapi.h>
#include <sstream>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

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

  HANDLE hProcess = OpenProcess(PROCESS_VM_READ, false, pid_);
  if (hProcess == NULL) {
    Utility::DebugLog("*** Error : OpenProcess failed ***\nPID: %d\n%s (errno=%d)\n", pid_, strerror(errno), errno);
    return 0;
  }

  SIZE_T bytesRead;
  BOOL result = ReadProcessMemory(hProcess, (LPCVOID)src.GetStart().to_i(), dest, n, &bytesRead);
  CloseHandle(hProcess);

  if (!result) {
    Utility::DebugLog("*** Error : Memory Read failed ***\nAddress: %zx\n%s (errno=%d)\n", src.GetStart().to_i(),
                      strerror(errno), errno);
    return 0;
  } else if (bytesRead != n) {
    Utility::DebugLog("*** Error : Memory Read failed ***\nExpected length: "
                      "%zu\nRead length: %zu\nAddress: %zx\n",
                      n, bytesRead, src.GetStart().to_i());
  }

  return bytesRead;
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

  size_t n = dest.GetEnd().to_i() - dest.GetStart().to_i();

  HANDLE hProcess = OpenProcess(PROCESS_VM_WRITE | PROCESS_VM_OPERATION, false, pid_);
  if (hProcess == NULL) {
    Utility::DebugLog("*** Error : OpenProcess failed ***\nPID: %d\n%s (errno=%d)\n", pid_, strerror(errno), errno);
    return 0;
  }

  SIZE_T bytesWritten;
  BOOL result = WriteProcessMemory(hProcess, (LPVOID)dest.GetStart().to_i(), src, n, &bytesWritten);
  CloseHandle(hProcess);

  if (!result) {
    Utility::DebugLog("*** Error : Memory Write failed ***\nAddress: %zx\n%s (errno=%d)\n", dest.GetStart().to_i(),
                      strerror(errno), errno);
    return 0;
  } else if (bytesWritten != n) {
    Utility::DebugLog("*** Error : Memory Write failed ***\nExpected length: "
                      "%zu\nWrite length: %zu\nAddress: %zx\n",
                      n, bytesWritten, dest.GetStart().to_i());
  }

  return bytesWritten;
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

  HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid_);
  if (hProcess == NULL) {
    Utility::DebugLog("Can't open process with PID %d", pid_);
    return false;
  }

  MEMORY_BASIC_INFORMATION mbi;
  DWORD64 address = 0;
  char module_name[MAX_PATH] = "";
  char dev[6] = "00:00";
  unsigned long inode = 0;
  SIZE_T offset = 0;

  while (VirtualQueryEx(hProcess, (LPCVOID)address, &mbi, sizeof(mbi)) == sizeof(mbi)) {
    if (mbi.State == MEM_COMMIT) {
      HMODULE hMod;
      if (GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                            (LPCTSTR)mbi.BaseAddress, &hMod)) {
        GetModuleFileNameEx(hProcess, hMod, module_name, sizeof(module_name));
      } else {
        strcpy(module_name, "");
      }

      ss << std::hex << (DWORD64)mbi.BaseAddress << "-" << (DWORD64)mbi.BaseAddress + mbi.RegionSize << " ";

      char permissions[4] = "---";
      permissions[3] = '\0';

      // Windowsのメモリ属性をrwx形式にマッピングする
      if (mbi.Protect & (PAGE_READONLY | PAGE_READWRITE | PAGE_WRITECOPY | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE |
                         PAGE_EXECUTE_WRITECOPY)) {
        permissions[0] = 'r';
      }
      if (mbi.Protect & (PAGE_READWRITE | PAGE_WRITECOPY | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY)) {
        permissions[1] = 'w';
      }
      if (mbi.Protect & (PAGE_EXECUTE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY)) {
        permissions[2] = 'x';
      }

      ss << permissions << " " << std::hex << offset << " " << dev << " " << inode << " " << module_name << "\n";
    }
    address += mbi.RegionSize;
  }

  CloseHandle(hProcess);
  return true;
}