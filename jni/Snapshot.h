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
#include <stdio.h>
#include <sys/stat.h>
#include <vector>

#include "Address.h"
#include "Config.h"
#include "SnappedRange.h"

class Snapshot {
public:
  typedef std::vector<SnappedRange>::iterator iterator;

public:
  Snapshot() {}
  Snapshot(const char *filename) : _filename(filename) {}
  ~Snapshot() { ::remove(_filename.c_str()); }

  void clear() { ::remove(_filename.c_str()); }

  iterator begin() { return _saved.begin(); }
  iterator end() { return _saved.end(); }

  void push_back(const Range &range, const void *data) {
    size_t size = range.Size();
    off_t offset = push(data, size);
    _saved.push_back(SnappedRange(this, range, offset));
  }
  off_t push(const void *data, size_t size) {
    struct stat stbuf;
    off_t offset = stat(_filename.c_str(), &stbuf) != -1 ? stbuf.st_size : 0;
    FILE *file = fopen(_filename.c_str(), "ab+");
    ::fwrite((const char *)data, size, 1, file);
    fclose(file);
    return offset;
  }
  std::unique_ptr<uint8_t[]> pull(off_t offset, size_t size) const {
    std::unique_ptr<uint8_t[]> buf = std::make_unique<uint8_t[]>(size);
    FILE *file = fopen(_filename.c_str(), "rb");
    fseek(file, offset, SEEK_SET);
    ::fread(buf.get(), size, 1, file);
    fclose(file);
    return buf;
  }

private:
  std::string _filename = std::string(STORAGE_PATH) + "/mempatch_memory-snapshot";
  std::vector<SnappedRange> _saved;
};
