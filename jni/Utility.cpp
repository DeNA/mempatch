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
#include <assert.h>
#include <errno.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Utility.h"

namespace Utility {

// #ifdef __IS_NDK_BUILD__
// #include <android/log.h>
//     // Android用
//     void DebugLog(const char* format, ...)
//     {
//         char msg[65536]; // it may be buffer over flow
//         va_list args;
//
//         va_start( args, format );
//         vsnprintf(msg, sizeof(msg), format, args);
//         va_end( args );
//         __android_log_write(ANDROID_LOG_DEBUG, "mempatch", msg);
//         fprintf(stderr, "%s\n", msg);
//     }
// #else
// Linux用
void DebugLog(const char *format, ...) {
  va_list args;
  va_start(args, format);
  vfprintf(stderr, format, args);
  va_end(args);
  fprintf(stderr, "\n");
}
// #endif

void PrintErrnoString(const char *format, ...) {
  int errsv = errno;
  char msg[512];
  va_list args;

  va_start(args, format);
  vsnprintf(msg, sizeof(msg), format, args);
  va_end(args);
  DebugLog("%s(%d) : %s", strerror(errsv), errsv, msg);
}

std::string HexDump(size_t address, const char *comment, const uint8_t *data, size_t n, int indent) {
  std::string ret;
  char buffer[300];
  snprintf(buffer, 299, "Dump: %zx-%zx (%zd byte, %s)", address, address + n, n, comment);
  ret += buffer;
  for (size_t i = 0; i < n; i += 16) {
    ret += "\n";
    int index = 0;
    for (int j = 0; j < indent; j++) {
      buffer[index++] = ' ';
    }
    int s = std::min((size_t)16, n - i);
    snprintf(buffer + index, 299 - index, "%zx", address + i);
    index = strlen(buffer);
    for (int j = 0; j < 16; j++) {
      if (j % 2 == 0) {
        buffer[index++] = ' ';
      }
      if (j < s) {
        snprintf(buffer + index, 299 - index, "%02x", ((int)data[i + j] & 0xff));
        index += 2;
      } else {
        buffer[index++] = ' ';
        buffer[index++] = ' ';
      }
    }
    buffer[index++] = ' ';
    buffer[index++] = ' ';
    for (int j = 0; j < s; j++) {
      char c = data[i + j];
      if (c < 0x20 || 0x7f <= c) {
        c = '.';
      }
      buffer[index++] = c;
    }
    buffer[index] = 0;
    ret += buffer;
  }
  return ret;
}

std::vector<size_t> StrstrByRollingHash(const uint8_t *src, const uint8_t *str, size_t n, size_t l) {
  const uint32_t BASE = 1e+9 + 7;
  assert(src != nullptr);
  assert(str != nullptr);
  if (n < l || n == 0 || l == 0) {
    return std::vector<size_t>();
  }

  // strのローリングハッシュ作成
  uint32_t rolling_hash = 0;
  for (size_t i = 0; i < l; i++) {
    uint32_t v = str[i];
    rolling_hash = rolling_hash * BASE + v;
  }

  // ローリングハッシュのキャンセル用の値を作成
  uint32_t BASE_len = 1;
  for (size_t i = 0; i < l; i++) {
    BASE_len *= BASE;
  }

  // ローリングハッシュを使って出現位置を求める
  uint32_t hash = 0;
  std::vector<size_t> ret;
  if (memcmp(src, str, l) == 0) {
    ret.emplace_back(0);
  }
  for (size_t i = 0; i < l; i++) {
    uint32_t v = src[i];
    hash = hash * BASE + v;
  }
  for (size_t i = l; i < n; i++) {
    // 1文字ごとに処理
    uint32_t v = src[i];
    uint32_t prev_v = src[i - l];
    hash = hash * BASE + v - prev_v * BASE_len;
    if (hash != rolling_hash) {
      continue;
    }
    // 文字列が見つかった
    // 誤検知が発生する可能性があるので実際に比較して確認する
    size_t find_index = i - l + 1;
    if (memcmp(src + find_index, str, l) == 0) {
      ret.emplace_back(find_index);
    }
  }
  return ret;
}

std::vector<size_t> StrstrByFloatFuzzyLookup(const uint8_t *src, const uint8_t *str_from, size_t n, size_t l) {

  assert(src != nullptr);
  assert(str_from != nullptr);
  if (n < l || n == 0 || l == 0) {
    return std::vector<size_t>();
  }

  // 検索文字列を数字に戻す
  float min = 0.0f;
  float max = 0.0f;
  memcpy((uint8_t *)&min, str_from, 4);
  max = min + 1.05f;
  min = min - 0.55f; // 四捨五入のケース対応

  std::vector<size_t> ret;
  for (size_t i = 0; i < n - l; i++) {
    float v = 0.0f;
    memcpy((uint8_t *)&v, src + i, 4);

    if (v < min || v > max || isnan(v)) {
      continue;
    }

    // 文字列が見つかった
    // 誤検知が発生する可能性があるので実際に比較して確認する
    size_t find_index = i;
    ret.emplace_back(find_index);
  }
  return ret;
}

void ByteSerialize(FILE *fp, const std::vector<uint8_t> &byte) {
  fprintf(fp, "_%zd", byte.size());
  fprintf(fp, "_");
  for (size_t i = 0; i < byte.size(); i++) {
    fprintf(fp, "%c", byte[i]);
  }
}
std::vector<uint8_t> ByteDeSerialize(FILE *fp) {
  std::vector<uint8_t> byte;
  size_t length;
  fscanf(fp, "_%zd", &length);
  fscanf(fp, "_");
  byte.resize(length);
  for (size_t i = 0; i < length; i++) {
    fscanf(fp, "%c", &byte[i]);
  }
  return byte;
}
void StringSerialize(FILE *fp, const std::string &str) {
  fprintf(fp, "_%zd", str.size());
  fprintf(fp, "_");
  for (size_t i = 0; i < str.size(); i++) {
    fprintf(fp, "%c", str[i]);
  }
}
std::string StringDeSerialize(FILE *fp) {
  std::string str;
  size_t length;
  fscanf(fp, "_%zd", &length);
  fscanf(fp, "_");
  str.resize(length);
  for (size_t i = 0; i < length; i++) {
    fscanf(fp, "%c", &str[i]);
  }
  return str;
}
} // namespace Utility
