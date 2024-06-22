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
#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#define usleep(usec) Sleep((usec) / 1000) // 1ミリ秒以上のスリープをサポート
#else
#include <unistd.h>
#endif
#include "FreezeThread.h"
#include "Memory.h"
#include "Utility.h"

void FreezeThread::Start() {
  assert(!terminate_flag_.load());
  freeze_thread_ = std::thread(&FreezeThread::ThreadFunction, this);
  // プロンプトの>よりもStart Freezeの表示をを先にしたいので少し待つ
  usleep(10 * 1000);
}

void FreezeThread::Terminate() {
  if (terminate_flag_.load()) {
    return;
  }
  terminate_flag_.store(true);
  freeze_thread_.join();
}

void FreezeThread::ThreadFunction() const {
  const Address address = target_.GetAddress();
  const ChangeString change_string = target_.GetChangeString();
  const size_t start = address.to_i();
  const size_t end = address.to_i() + change_string.Size();
  Range range = Range(start, end, "");
  std::vector<uint8_t> value_ = change_string.GetRawValue();
  Utility::DebugLog("Start Freeze Thread\n    %zx : %s (%s)", start, change_string.GetValue().c_str(),
                    change_string.GetTypeString().c_str());
  // teminateするか書き込みに失敗するまで実行する
  while (!terminate_flag_.load()) {
    size_t size = ptrace_->Write(range, value_.data(), true);
    if (size != change_string.Size()) {
      break;
    }
    usleep(1000);
  }
  Utility::DebugLog("Terminate Freeze Thread\n    %zx : %s (%s)", start, change_string.GetValue().c_str(),
                    change_string.GetTypeString().c_str());
}
