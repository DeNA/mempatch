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
#include <algorithm>
#include <assert.h>

#include "ChangeString.h"
#include "Utility.h"

ChangeString::ChangeString(const std::string &type, const std::string &value) {
  if (!Init(type, value)) {
    if (type_ == Converter::Type::INVALID) {
      Utility::DebugLog("type '%s' is wrong. Run time error may occur.", type.c_str());
    } else {
      Utility::DebugLog("value's size is different. Run time error may occur.");
    }
  }
}

bool ChangeString::Init(const std::string &type, const std::string &value) {
  type_ = Converter::GetType(type);
  value_ = Converter::GetByte(type_, value);
  if (type_ == Converter::Type::INVALID) {
    return false;
  }
  return true;
}

bool ChangeString::Init(int value) {
  type_ = Converter::Type::INT_LITTLE_ENDIAN;
  value_ = Converter::IntToByte(value);
  return true;
}

void ChangeString::Serialize(FILE *fp) const {
  Utility::StringSerialize(fp, GetTypeString());
  Utility::ByteSerialize(fp, value_);
}

ChangeString ChangeString::DeSerialize(FILE *fp) {
  std::string type = Utility::StringDeSerialize(fp);
  std::vector<uint8_t> value = Utility::ByteDeSerialize(fp);
  return ChangeString(Converter::GetType(type), value);
}
