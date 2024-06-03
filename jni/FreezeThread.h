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

#include <atomic>
#include <future>
#include <memory>

#include "Address.h"
#include "Ptrace.h"

class FreezeThread {
public:
  FreezeThread() = delete;
  FreezeThread(FreezeThread const &) = delete;
  FreezeThread &operator=(FreezeThread const &) = delete;
  FreezeThread(std::shared_ptr<Ptrace> &ptrace, const TargetAddress &target)
      : ptrace_(ptrace), target_(target), terminate_flag_(false) {
    ;
  }
  ~FreezeThread() { Terminate(); }
  void Start();
  void Terminate();

private:
  void ThreadFunction() const;

  const std::shared_ptr<const Ptrace> ptrace_;
  std::thread freeze_thread_;
  const TargetAddress target_;
  std::atomic<bool> terminate_flag_;
};
