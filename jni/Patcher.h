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
#include <sstream>
#include <stdint.h>
#include <string>
#include <vector>

#include "Address.h"
#include "ChangeString.h"
#include "FreezeThread.h"
#include "Memory.h"
#include "Snapshot.h"

class Patcher {
public:
  enum class Mode {
    NOP,
    LOOKUP,
    FILTER,
    CHANGE,
    DIFF,
  };
  static std::string GetModeString(Mode mode); // string -> Mode
  static Mode GetMode(const std::string &str); // Mode -> string

  Patcher() { Init(-1, false); }
  explicit Patcher(int pid, bool without_ptrace) { Init(pid, without_ptrace); }
  ~Patcher() {
    Exit();
    memory_.reset();
    snapshot_.reset();
  }

  // Command
  bool Attach(const std::string &command, std::stringstream &sin);
  bool Detach(const std::string &command, std::stringstream &sin);

  bool Clear(const std::string &command, std::stringstream &sin);
  bool Process(const std::string &command, std::stringstream &sin);
  bool PairFilter(const std::string &command, std::stringstream &sin);
  bool Replace(const std::string &command, std::stringstream &sin);
  bool Diff(const std::string &command, std::stringstream &sin);
  bool Freeze(const std::string &command, std::stringstream &sin);
  bool FreezeTerminate(const std::string &command, std::stringstream &sin);

  bool Result(const std::string &command, std::stringstream &sin);
  bool Dump(const std::string &command, std::stringstream &sin);
  bool Exit(const std::string &command, std::stringstream &sin);

  bool Scope(const std::string &command, std::stringstream &sin);
  bool Save(const std::string &command, std::stringstream &sin);
  bool Load(const std::string &command, std::stringstream &sin);
  bool DumpAll(const std::string &command, std::stringstream &sin);
  bool Help(const std::string &command, std::stringstream &sin);
  bool SaveResult(const std::string &filename) const;
  bool OutputResult(FILE *fp) const;

  void FreezeTerminate();
  void Exit(); // プログラムの終了時の処理、detachする

  // Getter
  size_t GetMemorySize() const {
    size_t ret = 0;
    for (auto it = range_set_.begin(); it != range_set_.end(); it++) {
      ret += it->Size();
    }
    return ret;
  }
  int GetRangeSetSize() const { return range_set_.size(); }
  int GetTargetAddressSetSize() const { return addr_set_.size(); }

  static void PrintCommandUsage() {
    fprintf(stderr, "PatcherCommand:\n");

    fprintf(stderr, "  attach                   attach process\n");
    fprintf(stderr, "  detach                   detach process\n");
    fprintf(stderr, "  clear                    clear detection rule & found address\n");

    fprintf(stderr, "  lookup [rule]            lookup memory under the rule\n");
    fprintf(stderr, "  filter [rule]            filter found address under the rule\n");
    fprintf(stderr, "  pair_filter [rule] cnt   filter by nearest another "
                    "value (e.g. lookup HP and filter by MP)\n");
    fprintf(stderr, "  change [rule]            replace found address under the rule\n");
    fprintf(stderr, "  replace [hex] [rule]     replace specific address under the rule\n");
    fprintf(stderr, "  freeze  [hex] [rule]     freeze target address\n");
    fprintf(stderr, "  freeze_terminate         stop & kill all freeze request\n");

    fprintf(stderr, "  scope [ascii]            set range scope (e.g. scope "
                    "[anon:libc_malloc])\n");
    fprintf(stderr, "  result                   output Lookup result\n");
    fprintf(stderr, "  dump [hexint] [hexint]   dump memory (e.g. dump "
                    "7f33f6963005 20) \n");
    fprintf(stderr, "  diff [start|lower|upper|same|change]\n");
    fprintf(stderr, "                           memory diff filter\n");
    fprintf(stderr, "  exit(quit)               exit mempatch\n");
    fprintf(stderr, "  save [path]              save current state to a file\n");
    fprintf(stderr, "  load [path]              load previous state to a file\n");
    fprintf(stderr, "  dumpall [path]           dump all memory data to a file\n");
    fprintf(stderr, "  help                     print this message\n");
    fprintf(stderr, "  #  comment\n");
    fprintf(stderr, "  // comment\n");
    ChangeString::PrintCommandUsage();
  }

private:
  void Init(int pid, bool without_ptrace) {
    last_process_time_ = -1;
    memory_ = std::make_shared<Memory>(pid, without_ptrace);
  }
  bool CreateRangeSet();
  bool Process(const Mode mode, const ChangeString &change_str);
  bool LookUp(const ChangeString &change_str);
  bool Filter(const ChangeString &change_str);
  bool ReplaceAll(const ChangeString &change_str);
  bool Replace(const TargetAddress &target_address, const ChangeString &change_str);

  int last_process_time_;
  RangeSet range_set_;
  std::vector<TargetAddress> addr_set_;
  std::vector<std::unique_ptr<FreezeThread>> freeze_set_;
  std::shared_ptr<Memory> memory_;
  std::unique_ptr<Snapshot> snapshot_;
  std::string range_scope_;

  bool DumpAll(const std::string &filename);
  bool DumpRange(FILE *fp, const Range &range);
  std::unique_ptr<uint8_t[]> LoadDumpRange(FILE *fp, const Range &range);
  void Serialize(FILE *fp) const;
  void DeSerialize(FILE *fp);
};
