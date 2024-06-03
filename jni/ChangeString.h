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
#include <string>

#include "Converter.h"

class ChangeString {
public:
  ChangeString() : type_(Converter::Type::INVALID) { ; }
  ChangeString(const Converter::Type &type, const std::vector<uint8_t> &value) : type_(type), value_(value) { ; }
  ChangeString(const std::string &type, const std::string &value);
  ChangeString(int value) { Init(value); }
  bool Init(const std::string &type, const std::string &value);
  bool Init(int value);
  virtual ~ChangeString() { ; }
  const std::vector<uint8_t> &GetRawValue() const { return value_; }
  std::string GetHexValue() const { return Converter::ByteToHex(value_); }
  std::string GetValue() const { return Converter::GetString(type_, value_); }

  size_t Size() const { return value_.size(); }
  const Converter::Type GetType() const { return type_; }
  const std::string GetTypeString() const { return Converter::GetTypeString(type_); }

  static void PrintCommandUsage() {
    fprintf(stderr, "Replace Rule\n");
    fprintf(stderr, "  ascii [value]\n");
    fprintf(stderr, "  utf16 [value]\n");
    fprintf(stderr, "  utf32 [value]\n");
    fprintf(stderr, "  hex [value]\n");
    fprintf(stderr, "  int [value]\n");
    fprintf(stderr, "  int_big [value]\n");
    fprintf(stderr, "  long [value]\n");
    fprintf(stderr, "  long_big [value]\n");
    fprintf(stderr, "  double [value]\n");
    fprintf(stderr, "  double_big [value]\n");
    fprintf(stderr, "  float [value]\n");
    fprintf(stderr, "  float_big [value]\n");
    fprintf(stderr, "  float_fuzzy [value]\n");
  }

  bool operator<(const ChangeString &rhs) const { return value_ < rhs.value_; }
  bool operator>(const ChangeString &rhs) const { return value_ > rhs.value_; }
  bool operator<=(const ChangeString &rhs) const { return value_ <= rhs.value_; }
  bool operator>=(const ChangeString &rhs) const { return value_ >= rhs.value_; }
  bool operator==(const ChangeString &rhs) const { return value_ == rhs.value_; }
  bool operator!=(const ChangeString &rhs) const { return value_ != rhs.value_; }
  void Serialize(FILE *fp) const;
  static ChangeString DeSerialize(FILE *fp);

private:
  Converter::Type type_;
  std::vector<uint8_t> value_; // lookup, filter,
                               // changeなどの処理が終わった後にメモリに入っている値

  std::string GetString(const std::string &str) const;
};
