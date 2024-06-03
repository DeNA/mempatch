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

#include <set>
#include <stdint.h>
#include <stdlib.h>
#include <string>
#include <vector>

#include "Converter.h"

namespace Utility {
void DebugLog(const char *format, ...);
void PrintErrnoString(const char *format, ...);
std::vector<size_t> StrstrByRollingHash(const uint8_t *src, const uint8_t *str, size_t n, size_t l);
std::vector<size_t> StrstrByFloatFuzzyLookup(const uint8_t *src, const uint8_t *str_from, size_t n, size_t l);
std::string HexDump(size_t address, const char *comment, const uint8_t *data, size_t n, int indent);

void ByteSerialize(FILE *fp, const std::vector<uint8_t> &byte);
std::vector<uint8_t> ByteDeSerialize(FILE *fp);
void StringSerialize(FILE *fp, const std::string &str);
std::string StringDeSerialize(FILE *fp);
template <class T> void SetSerialize(FILE *fp, const std::set<T> &items) {
  fprintf(fp, "_%zd", items.size());
  for (auto it = items.begin(); it != items.end(); it++) {
    it->Serialize(fp);
  }
}
template <class T> void VectorSerialize(FILE *fp, const std::vector<T> &items) {
  fprintf(fp, "_%zd", items.size());
  for (auto it = items.begin(); it != items.end(); it++) {
    it->Serialize(fp);
  }
}
template <class T> std::set<T> SetDeSerialize(FILE *fp) {
  std::set<T> ret;
  size_t length;
  fscanf(fp, "_%zd", &length);
  for (size_t i = 0; i < length; i++) {
    ret.insert(T::DeSerialize(fp));
  }
  return ret;
}
template <class T> std::vector<T> VectorDeSerialize(FILE *fp) {
  std::vector<T> ret;
  size_t length;
  fscanf(fp, "_%zd", &length);
  for (size_t i = 0; i < length; i++) {
    ret.emplace_back(T::DeSerialize(fp));
  }
  return ret;
}
} // namespace Utility
