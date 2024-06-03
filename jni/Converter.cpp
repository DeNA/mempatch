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
#include <map>

#include "Converter.h"
#include "Utility.h"

namespace Converter {
Type GetType(const std::string &str) {
  std::map<std::string, Type> temp = {
      {"ascii", Type::ASCII},
      {"utf16", Type::UTF16},
      {"utf32", Type::UTF32},
      {"hex", Type::HEX},
      {"int", Type::INT_LITTLE_ENDIAN},
      {"int_big", Type::INT_BIG_ENDIAN},
      {"long", Type::LONG_LITTLE_ENDIAN},
      {"long_big", Type::LONG_BIG_ENDIAN},
      {"double", Type::DOUBLE_LITTLE_ENDIAN},
      {"double_big", Type::DOUBLE_BIG_ENDIAN},
      {"float", Type::FLOAT_LITTLE_ENDIAN},
      {"float_big", Type::FLOAT_BIG_ENDIAN},
      {"float_fuzzy", Type::FLOAT_FUZZY_LITTLE_ENDIAN},
  };
  if (!temp.count(str)) {
    return Type::INVALID;
  }
  return temp[str];
}
std::string GetTypeString(const Type &type) {
  std::map<Type, std::string> temp = {
      {Type::ASCII, "ascii"},
      {Type::UTF16, "utf16"},
      {Type::UTF32, "utf32"},
      {Type::HEX, "hex"},
      {Type::INT_LITTLE_ENDIAN, "int"},
      {Type::INT_BIG_ENDIAN, "int_big"},
      {Type::LONG_LITTLE_ENDIAN, "long"},
      {Type::LONG_BIG_ENDIAN, "long_big"},
      {Type::DOUBLE_LITTLE_ENDIAN, "double"},
      {Type::DOUBLE_BIG_ENDIAN, "double_big"},
      {Type::FLOAT_LITTLE_ENDIAN, "float"},
      {Type::FLOAT_BIG_ENDIAN, "float_big"},
      {Type::FLOAT_FUZZY_LITTLE_ENDIAN, "float_fuzzy"},
  };
  if (!temp.count(type)) {
    return "INVALID";
  }
  return temp[type];
}
std::string GetString(const Type &type, const std::vector<uint8_t> &byte) {
  switch (type) {
  case Type::ASCII:
    return ByteToAscii(byte);
  case Type::UTF16:
    return Utf16ToAscii(byte);
  case Type::UTF32:
    return Utf32ToAscii(byte);
  case Type::HEX:
    return ByteToHex(byte);
  case Type::INT_LITTLE_ENDIAN:
    return ByteToIntstr(byte);
  case Type::INT_BIG_ENDIAN: {
    std::vector<uint8_t> temp_byte = byte;
    std::reverse(temp_byte.begin(), temp_byte.end());
    return ByteToIntstr(temp_byte);
  }
  case Type::LONG_LITTLE_ENDIAN:
    return ByteToLongstr(byte);
  case Type::LONG_BIG_ENDIAN: {
    std::vector<uint8_t> temp_byte = byte;
    std::reverse(temp_byte.begin(), temp_byte.end());
    return ByteToLongstr(temp_byte);
  }
  case Type::DOUBLE_LITTLE_ENDIAN:
    return ByteToDoublestr(byte);
  case Type::DOUBLE_BIG_ENDIAN: {
    std::vector<uint8_t> temp_byte = byte;
    std::reverse(temp_byte.begin(), temp_byte.end());
    return ByteToDoublestr(temp_byte);
  }
  case Type::FLOAT_LITTLE_ENDIAN:
    return ByteToFloatstr(byte);
  case Type::FLOAT_BIG_ENDIAN: {
    std::vector<uint8_t> temp_byte = byte;
    std::reverse(temp_byte.begin(), temp_byte.end());
    return ByteToFloatstr(temp_byte);
  }
  case Type::FLOAT_FUZZY_LITTLE_ENDIAN:
    return ByteToFloatstr(byte);
  default:
    assert(false);
  }
  return "INVALID_TYPE";
}
std::vector<uint8_t> GetByte(const Type &type, const std::string &str) {
  switch (type) {
  case Type::ASCII:
    return AsciiToByte(str);
  case Type::UTF16:
    return AsciiToUtf16(str);
  case Type::UTF32:
    return AsciiToUtf32(str);
  case Type::HEX:
    return HexToByte(str);
  case Type::INT_LITTLE_ENDIAN:
    return IntToByte(atoi(str.c_str()));
  case Type::INT_BIG_ENDIAN: {
    std::vector<uint8_t> ret = IntToByte(atoi(str.c_str()));
    std::reverse(ret.begin(), ret.end());
    return ret;
  }
  case Type::LONG_LITTLE_ENDIAN:
    return LongToByte(atol(str.c_str()));
  case Type::LONG_BIG_ENDIAN: {
    std::vector<uint8_t> ret = LongToByte(atol(str.c_str()));
    std::reverse(ret.begin(), ret.end());
    return ret;
  }
  case Type::DOUBLE_LITTLE_ENDIAN:
    return DoubleToByte(atof(str.c_str()));
  case Type::DOUBLE_BIG_ENDIAN: {
    std::vector<uint8_t> ret = DoubleToByte(atof(str.c_str()));
    std::reverse(ret.begin(), ret.end());
    return ret;
  }
  case Type::FLOAT_LITTLE_ENDIAN:
    return FloatToByte(atof(str.c_str()));
  case Type::FLOAT_BIG_ENDIAN: {
    std::vector<uint8_t> ret = FloatToByte(atof(str.c_str()));
    std::reverse(ret.begin(), ret.end());
    return ret;
  }
  case Type::FLOAT_FUZZY_LITTLE_ENDIAN:
    return FloatToByte(atof(str.c_str()));
  default:
    break;
  }
  return AsciiToByte("INVALID_TYPE");
}

std::vector<uint8_t> AsciiToByte(const std::string &str) {
  return std::vector<uint8_t>(str.data(), str.data() + str.size());
}
std::string ByteToAscii(const std::vector<uint8_t> &byte) {
  return std::string(byte.data(), byte.data() + byte.size());
}

std::vector<uint8_t> HexToByte(const std::string &hex) {
  std::vector<uint8_t> ret;
  int v = 0;
  int cnt = 0;
  for (auto it = hex.begin(); it != hex.end(); it++) {
    int c = tolower(*it);
    if (isspace(c)) {
      continue;
    }
    if (('0' <= c && c <= '9') && ('a' <= c && c <= 'f')) {
      Utility::DebugLog("'%s' is not hex string", hex.c_str());
      return AsciiToByte("");
    }
    v = v * 16 + (isdigit(c) ? c - '0' : c - 'a' + 10);
    cnt++;
    if (cnt % 2 == 0) {
      ret.emplace_back(v);
      v = 0;
    }
  }
  return ret;
}

std::string ByteToHex(const std::vector<uint8_t> &byte) {
  std::string ret;
  for (auto it = byte.begin(); it != byte.end(); it++) {
    int b = (*it & 0x000000ff);
    int vs[2] = {b / 16, b % 16};
    for (int i = 0; i < 2; i++) {
      int c = (vs[i] < 10 ? vs[i] + '0' : vs[i] + 'a' - 10);
      ret.push_back(c);
    }
  }
  return ret;
}

// little endianで返す
std::vector<uint8_t> IntToByte(int v) { return std::vector<uint8_t>((uint8_t *)&v, ((uint8_t *)&v) + 4); }

// little endianを想定
int ByteToInt(const std::vector<uint8_t> &byte) { return *(int *)byte.data(); }

// little endianを想定
std::string ByteToIntstr(const std::vector<uint8_t> &byte) {
  int v = ByteToInt(byte);
  char str[20];
  snprintf(str, 19, "%d", v);
  std::string ret = str;
  return ret;
}

std::vector<uint8_t> LongToByte(long v) {
  std::vector<uint8_t> ret;
  for (int i = 0; i < (int)sizeof(long); i++) {
    ret.emplace_back(v & 0x000000ff);
    v >>= 8;
  }
  return ret;
}

// little endianを想定
long ByteToLong(const std::vector<uint8_t> &byte) {
  long ret = 0;
  for (int i = 0; i < (int)sizeof(long); i++) {
    ret |= (byte[i] & 0xff) << (i * 8);
  }
  return ret;
}

// little endianを想定
std::string ByteToLongstr(const std::vector<uint8_t> &byte) {
  long v = ByteToLong(byte);
  char str[20];
  snprintf(str, 19, "%ld", v);
  std::string ret = str;
  return ret;
}

// little endianで返す
std::vector<uint8_t> DoubleToByte(double v) {
  union DoubleByte {
    double d;
    uint8_t cs[sizeof(double)];
  };
  DoubleByte x;
  x.d = v;
  std::vector<uint8_t> ret;
  for (int i = 0; i < (int)sizeof(double); i++) {
    ret.emplace_back(x.cs[i] & 0xff);
  }
  return ret;
}

// little endianを想定
double ByteToDouble(const std::vector<uint8_t> &byte) {
  union DoubleByte {
    double d;
    uint8_t cs[sizeof(double)];
  };
  DoubleByte x;
  for (int i = 0; i < (int)sizeof(double); i++) {
    x.cs[i] = byte[i] & 0x000000ff;
  }
  return x.d;
}

// little endianを想定
std::string ByteToDoublestr(const std::vector<uint8_t> &byte) {
  double v = ByteToDouble(byte);
  char str[50];
  snprintf(str, 49, "%.8f", v);
  std::string ret = str;
  return ret;
}

// little endianで返す
std::vector<uint8_t> FloatToByte(float v) {
  union FloatByte {
    float d;
    uint8_t cs[sizeof(float)];
  };
  FloatByte x;
  x.d = v;
  std::vector<uint8_t> ret;
  for (int i = 0; i < (int)sizeof(float); i++) {
    ret.emplace_back(x.cs[i] & 0xff);
  }
  return ret;
}

// little endianを想定
float ByteToFloat(const std::vector<uint8_t> &byte) {
  union FloatByte {
    float d;
    uint8_t cs[sizeof(float)];
  };
  FloatByte x;
  for (int i = 0; i < (int)sizeof(float); i++) {
    x.cs[i] = byte[i] & 0x000000ff;
  }
  return x.d;
}

// little endianを想定
std::string ByteToFloatstr(const std::vector<uint8_t> &byte) {
  float v = ByteToFloat(byte);
  char str[50];
  snprintf(str, 49, "%.8f", v);
  std::string ret = str;
  return ret;
}

// big endianを想定
std::vector<uint8_t> AsciiToUtf16(const std::string &str) {
  std::vector<uint8_t> ret;
  for (auto it = str.begin(); it != str.end(); it++) {
    ret.emplace_back('\0');
    ret.emplace_back(*it);
  }
  return ret;
}

// big endianを想定
std::string Utf16ToAscii(const std::vector<uint8_t> &byte) {
  std::string ret;
  int cnt = 0;
  for (auto it = byte.begin(); it != byte.end(); it++) {
    cnt++;
    if (cnt % 2 == 0) {
      ret.push_back(*it);
    }
  }
  return ret;
}

// big endianを想定
std::vector<uint8_t> AsciiToUtf32(const std::string &str) {
  std::vector<uint8_t> ret;
  for (auto it = str.begin(); it != str.end(); it++) {
    ret.emplace_back('\0');
    ret.emplace_back('\0');
    ret.emplace_back('\0');
    ret.emplace_back(*it);
  }
  return ret;
}

// big endianを想定
std::string Utf32ToAscii(const std::vector<uint8_t> &byte) {
  std::string ret;
  int cnt = 0;
  for (auto it = byte.begin(); it != byte.end(); it++) {
    cnt++;
    if (cnt % 4 == 0) {
      ret.push_back(*it);
    }
  }
  return ret;
}

std::vector<uint8_t> RawByteToByte(const uint8_t *byte, size_t len) { return std::vector<uint8_t>(byte, byte + len); }
}; // namespace Converter

// #include <iostream>
// int main() {
//     // double hoge = 1.1;
//     int hoge = 123456789;
//     std::vector<uint8_t> byte = Converter::IntToByte(hoge);
//     for (int i = 0; i < sizeof(hoge); i++) {
//         printf("%02xd", byte[i] & 0xff);
//     }
//     puts("");
//     std::cout << Converter::ByteToIntstr(byte) << std::endl;
//     std::cout << Converter::ByteToInt(byte) << std::endl;
// }

// #include <iostream>
// int main() {
// 	std::string abcd = "abcd";
// 	std::string utf16_abcd, utf16, rev_utf16;
//     ChangeString str;
//     str.HexToByte("0061006200630064", utf16, 0x00);
// 	str.AsciiToUtf16(abcd, utf16_abcd, 0x00);
// 	str.Utf16ToAscii(utf16, rev_utf16, 0x00);
//     ChangeString str2("utf16", abcd, abcd);
//     std::cout << str2.Compare(utf16_abcd.c_str()) << std::endl;
//     std::cout << str2.before_ << std::endl;
//
// 	assert(utf16_abcd == utf16);
// 	assert(rev_utf16 == abcd);
//
// 	std::string hex_abcd = "61626364";
// 	std::string abcd = "abcd";
// 	int int_abcd = 1684234849;
// 	std::cout << HexToByte(hex_abcd) << std::endl;
// 	std::cout << ByteToHex(abcd) << std::endl;
// 	std::cout << int_to_byte(int_abcd) << std::endl;
// 	std::cout << byte_to_int(abcd) << std::endl;
//
// 	std::string sample1 = "00102030405060708090ff";
// 	std::string str = hex_to_byte(sample1);
// 	int anss[11] = { 0, 16, 32, 48, 64, 80, 96, 112, 128, 144, 255 };
// 	for (int i = 0; i < 11; i++) {
// 		printf("%d %d\n", (unsigned char)str[i], anss[i]);
// 		assert((str[i] & 0x000000ff) == anss[i]);
// 	}
// 	std::string sample2 = byte_to_hex(str);
// 	std::cout << sample1 << " " << sample2 << std::endl;
// 	assert(sample1 == sample2);
//
// 	int large1 = 1e+9 + 7;
// 	std::string converted = int_to_byte(large1);
// 	int large2 = byte_to_int(converted);
// 	std::cout << large1 << " " << large2 << std::endl;
// 	assert(large1 == large2);
//
// 	std::string utf16_abcd = hex_to_byte("0061006200630064");
// 	std::string utf16 = ascii_to_utf16(abcd);
// 	std::string rev_utf16 = utf16_to_ascii(utf16);
// 	assert(utf16_abcd == utf16);
// 	assert(rev_utf16 == abcd);
//
// 	std::string utf32_abcd =
// hex_to_byte("00000061000000620000006300000064"); 	std::string utf32 =
// ascii_to_utf32(abcd); 	std::string rev_utf32 = utf32_to_ascii(utf32);
// 	assert(utf32_abcd == utf32);
// 	assert(rev_utf32 == abcd);
// }
