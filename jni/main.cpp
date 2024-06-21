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
#include <memory>
#include <signal.h>
#include <stdlib.h>
#include <string>
#include <unistd.h>

#include "Config.h"
#include "LineReader.h"
#include "Patcher.h"
const char *VERSION = "1.3.1";

void Usage(const char *exepath) {
  fprintf(stderr, "Usage: %s -p pid\n", exepath);
  fprintf(stderr, "Version: mempatch v%s\n", VERSION);
  fprintf(stderr, "Options:\n");
  fprintf(stderr, "  -h        Print this message\n");
  fprintf(stderr, "  -w        Without ptrace\n");
  fprintf(stderr, "  -l        Windows mode\n");
  fprintf(stderr, "  -p pid    Set process ID to attach\n");
  fprintf(stderr, "\n");
  Patcher::PrintCommandUsage();
  exit(1);
}

static std::unique_ptr<Patcher> patcher; // signal用

void sigcatch(int sig) {
  static bool first = true;
  if (first) {
    first = false;
    fprintf(stderr, "catch signal %d & Exit mempach\n", sig);
    patcher->Exit();
  }
  exit(1);
}

int main(int argc, char *argv[]) {
  const char *exepath = argv[0];
  bool without_ptrace = false;
  bool windows = false;
  int pid = -1;
  int result;
  while ((result = getopt(argc, argv, "hwlp:")) != -1) {
    switch (result) {
    case 'p':
      pid = atoi(optarg);
      break;
    case 'w':
      without_ptrace = true;
      break;
    case 'l':
      windows = true;
      break;
    case 'h':
    default:
      Usage(exepath);
      break;
    }
  }
  if (pid == -1) {
    Usage(exepath);
  }

  if (without_ptrace) {
    fprintf(stdout, "Without Ptrace Mode\n");
  }
  if (windows) {
    fprintf(stdout, "Windows Mode\n");
  }
  patcher = std::make_unique<Patcher>(pid, without_ptrace);

  if (SIG_ERR == signal(SIGHUP, sigcatch) || SIG_ERR == signal(SIGINT, sigcatch) ||
      SIG_ERR == signal(SIGQUIT, sigcatch) || SIG_ERR == signal(SIGSEGV, sigcatch)) {
    fprintf(stderr, "failed to set signal handler\n");
    exit(1);
  }

  // const std::string change_filename("/sdcard/changeset");
  // const std::string result_filename("/sdcard/changeresult");

  LineReader::Dispatch(*patcher, windows);
  patcher->Exit();

  return 0;
}
