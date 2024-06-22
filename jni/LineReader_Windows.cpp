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
#include "Config.h"
#include "LineReader.h"
#include "Utility.h"
#include "linenoise/linenoise.hpp"
#include <iostream>
#include <string.h>


std::map<std::string, LineReader::PatcherCommandFunc> LineReader::commands;

void completion(const std::string &buf, std::vector<std::string> &completions) {
  // TODO
}

void LineReader::InitCommandMap() {
  commands.clear();

  commands["attach"] = &Patcher::Attach;
  commands["detach"] = &Patcher::Detach;

  commands["clear"] = &Patcher::Clear;
  commands["lookup"] = &Patcher::Process;
  commands["filter"] = &Patcher::Process;
  commands["pair_filter"] = &Patcher::PairFilter;
  commands["change"] = &Patcher::Process;
  commands["replace"] = &Patcher::Replace;
  commands["freeze"] = &Patcher::Freeze;
  commands["freeze_terminate"] = &Patcher::FreezeTerminate;
  commands["diff"] = &Patcher::Diff;

  commands["scope"] = &Patcher::Scope;
  commands["save"] = &Patcher::Save;
  commands["load"] = &Patcher::Load;
  commands["result"] = &Patcher::Result;
  commands["dump"] = &Patcher::Dump;
  commands["dumpall"] = &Patcher::DumpAll;
  commands["help"] = &Patcher::Help;
  commands["exit"] = &Patcher::Exit;
  commands["quit"] = &Patcher::Exit;

  commands["l"] = commands["lookup"];
  commands["f"] = commands["filter"];
  commands["c"] = commands["change"];
}

bool LineReader::Dispatch(Patcher &patcher, bool windows) {
  InitCommandMap();

  // Set the completion callback
  linenoise::SetCompletionCallback(completion);

  // Load history from file
  std::string history_path = std::string(STORAGE_PATH) + "/mempatch_history.txt";
  linenoise::LoadHistory(history_path.c_str());

  std::cout << "\nPlease Input Command\n";

  while (true) {
    std::string line;
    if (!windows) {
      if (!linenoise::Readline("> ", line)) {
        break;
      }
    } else {
      std::cout << "> ";
      std::getline(std::cin, line);
      if (line.empty()) {
        continue;
      }
    }

    linenoise::AddHistory(line.c_str());
    linenoise::SaveHistory(history_path.c_str());
    std::stringstream sin(line);

    // コマンドを取得
    std::string command;
    if (!(sin >> command)) {
      continue;
    }

    // コマンドを確認して処理を実行
    for (auto &c : command) {
      c = tolower(c);
    }

    if (command.empty() || command[0] == '#' || command[0] == '/') {
      continue;
    } else if (!commands.count(command)) {
      Utility::DebugLog("command '%s' is invalid", command.c_str());
      continue;
    } else {
      if (!(patcher.*commands[command])(command, sin)) {
        Utility::DebugLog("argument is invalid or failed to process");
      }
      if (commands[command] == commands["exit"]) {
        break;
      }
    }
  }
  return true;
}

char *LineReader::GetLine(bool windows) {
  std::string line;
  if (!windows) {
    if (linenoise::Readline("> ", line)) {
      return strdup(line.c_str());
    }
  } else {
    std::cout << "> ";
    std::getline(std::cin, line);
    return strdup(line.c_str());
  }
  return nullptr;
}
