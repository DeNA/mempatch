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

#include <map>
#include <sstream>
#include <string>

#include "Patcher.h"

class LineReader {
public:
  static bool Dispatch(Patcher &patcher, bool windows);

private:
  static void InitCommandMap();
  static char *GetLine(bool windows);

  // Patcherのコマンドを実行するメンバ関数への関数ポインタの定義
  typedef bool (Patcher::*PatcherCommandFunc)(const std::string &, std::stringstream &);
  static std::map<std::string, PatcherCommandFunc> commands;
};
