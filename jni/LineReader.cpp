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
#include <string.h>

#include "LineReader.h"
#include "Utility.h"
#include "linenoise/linenoise.h"

std::map<std::string, LineReader::PatcherCommandFunc> LineReader::commands;

void completion(const char *buf, linenoiseCompletions *lc) {
  // TODO
  // if (buf[0] == 'h') {
  //     linenoiseAddCompletion(lc,"hello");
  //     linenoiseAddCompletion(lc,"hello there");
  // }
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

  /* Set the completion callback. This will be called every time the
   * user uses the <tab> key. */
  linenoiseSetCompletionCallback(completion);

  /* Load history from file. The history file is just a plain text file
   * where entries are separated by newlines. */
  linenoiseHistoryLoad("/sdcard/mempatch_history.txt"); /* Load the history at startup */

  printf("\nPlease Input Command\n");

  char *buffer = nullptr;
  while ((buffer = GetLine(windows)) != nullptr) {
    linenoiseHistoryAdd(buffer);                          /* Add to the history. */
    linenoiseHistorySave("/sdcard/mempatch_history.txt"); /* Save the history on disk. */
    std::stringstream sin(buffer);
    free(buffer);
    buffer = nullptr;

    // コマンドを取得
    std::string command;
    if (!(sin >> command)) {
      continue;
    }

    // コマンドを確認して処理を実行
    for (auto it = command.begin(); it != command.end(); it++) {
      *it = tolower(*it);
    } // range based forが使えない
    if (command.size() == 0 || command[0] == '#' || command[0] == '/') {
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
  if (!windows) {
    return linenoise("> ");
  }
  printf("> ");
  fflush(stdout);
  const int MAX_BUFFER = 4096;
  char buf[MAX_BUFFER];
  if (fgets(buf, MAX_BUFFER, stdin) == nullptr) {
    return nullptr;
  }
  int len = strlen(buf);
  while (len && (buf[len - 1] == '\n' || buf[len - 1] == '\r')) {
    len--;
    buf[len] = '\0';
  }
  return strdup(buf);
}
