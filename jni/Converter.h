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

#include <stdint.h>
#include <string>
#include <vector>

namespace Converter {
enum class Type {
  ASCII,
  UTF16,
  UTF32,
  HEX,
  INT_LITTLE_ENDIAN,
  INT_BIG_ENDIAN,
  LONG_LITTLE_ENDIAN,
  LONG_BIG_ENDIAN,
  DOUBLE_LITTLE_ENDIAN,
  DOUBLE_BIG_ENDIAN,
  FLOAT_LITTLE_ENDIAN,
  FLOAT_BIG_ENDIAN,
  FLOAT_FUZZY_LITTLE_ENDIAN,
  INVALID,
};
Type GetType(const std::string &str);
std::string GetTypeString(const Type &type);

std::string GetString(const Type &type, const std::vector<uint8_t> &byte);
std::vector<uint8_t> GetByte(const Type &type, const std::string &str);

std::vector<uint8_t> AsciiToByte(const std::string &str);
std::string ByteToAscii(const std::vector<uint8_t> &byte);

std::vector<uint8_t> HexToByte(const std::string &hex);
std::string ByteToHex(const std::vector<uint8_t> &byte);

// little endianで返す
std::vector<uint8_t> IntToByte(int v);
int ByteToInt(const std::vector<uint8_t> &byte);
int HexToInt(const std::string &str);
std::string ByteToIntstr(const std::vector<uint8_t> &byte);

// little endianを想定
std::vector<uint8_t> LongToByte(long v);
long ByteToLong(const std::vector<uint8_t> &byte);
std::string ByteToLongstr(const std::vector<uint8_t> &byte);

// little endianを想定
std::vector<uint8_t> DoubleToByte(double v);
double ByteToDouble(const std::vector<uint8_t> &byte);
std::string ByteToDoublestr(const std::vector<uint8_t> &byte);

// little endianを想定
std::vector<uint8_t> FloatToByte(float v);
float ByteToFloat(const std::vector<uint8_t> &byte);
std::string ByteToFloatstr(const std::vector<uint8_t> &byte);

// big endianを想定
std::vector<uint8_t> AsciiToUtf16(const std::string &str);
std::string Utf16ToAscii(const std::vector<uint8_t> &byte);
std::vector<uint8_t> AsciiToUtf32(const std::string &str);
std::string Utf32ToAscii(const std::vector<uint8_t> &byte);

// Readとかから取ったやつの変換
std::vector<uint8_t> RawByteToByte(const uint8_t *byte, size_t len);
}; // namespace Converter
