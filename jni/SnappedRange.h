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
#include <stdint.h>
#include <stdlib.h>

#include "Address.h"

class Snapshot;

class SnappedRange {
public:
  SnappedRange(const Snapshot *snapshot, const Range &range, off_t fileoff)
      : _parent(snapshot), _range(range), _fileoff(fileoff) {}
  Range range() const { return _range; }
  std::unique_ptr<uint8_t[]> data() const;

private:
  const Snapshot *_parent;
  Range _range;
  off_t _fileoff;
};
