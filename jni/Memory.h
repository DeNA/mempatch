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
#pragma once

#include <memory>
#include <set>
#include <stdint.h>

#include "Address.h"

class Memory {
public:
  Memory() : pid_(-1), attached_(false) { ; }
  explicit Memory(int pid, bool without_ptrace) : pid_(pid), attached_(false), without_ptrace_(without_ptrace) { ; }
  virtual ~Memory() {
    if (pid_ != -1) {
      Detach();
    }
    cache_.reset();
  }

  int GetPid() const { return pid_; }
  bool IsAttached() const { return attached_; }
  bool Attach();
  bool Detach();
  // size_t ReadByPeekData(uint8_t *dest, const Address &src) const;
  // size_t ReadByPeekData(uint8_t *dest, const Range &src) const;
  size_t Read(uint8_t *dest, const Range &src) const;
  size_t ReadWithCache(uint8_t *dest, const Range &src, const Range &parent_range) const;
  size_t WriteByPokeData(const Address &dest, long value) const;
  size_t WriteByPokeData(const Range &dest, const uint8_t *src, bool freeze_request) const;
  size_t Write(const Range &dest, const uint8_t *src, bool freeze_request) const;
  void Dump(const Range &src) const;
  bool GenerateMaps(std::stringstream &ss);

private:
  int pid_;
  bool attached_;
  bool without_ptrace_;
  std::set<int> thread_ids_;

  // cacheはAttachした際にclearされる
  mutable Range cache_range_;
  mutable std::unique_ptr<uint8_t[]> cache_;

  void LoadThreadIDs();
  void ClearCache();
};
