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

#include "Address.h"
#include "Utility.h"

const std::string Address::GetComment(const std::set<Range> &range_set) const {
  for (auto it = range_set.begin(); it != range_set.end(); it++) {
    if (it->GetStart() <= *this && *this < it->GetEnd()) {
      return it->GetComment();
    }
  }
  return "";
}
void Address::Serialize(FILE *fp) const { fprintf(fp, "_%zd", addr_); }
Address Address::DeSerialize(FILE *fp) {
  size_t addr;
  fscanf(fp, "_%zd", &addr);
  return Address(addr);
}

Address Range::Fit(const std::set<Range> &range_set, const Address &address) {
  for (auto it = range_set.begin(); it != range_set.end(); it++) {
    if (it->GetStart() <= address && address < it->GetEnd()) {
      return address;
    }
  }
  return Address(0);
}

Range Range::Fit(const std::set<Range> &range_set, const Range &range) {
  for (auto it = range_set.begin(); it != range_set.end(); it++) {
    if ((it->GetStart() <= range.GetStart() && range.GetStart() < it->GetEnd()) ||
        (range.GetStart() <= it->GetStart() && it->GetStart() < range.GetEnd())) {
      return Range(std::max(it->GetStart().to_i(), range.GetStart().to_i()),
                   std::min(it->GetEnd().to_i(), range.GetEnd().to_i()), range.GetComment());
    }
  }
  return Range(0, 0, "");
}
void Range::Serialize(FILE *fp) const {
  start_.Serialize(fp);
  end_.Serialize(fp);
  Utility::StringSerialize(fp, comment_);
}
Range Range::DeSerialize(FILE *fp) {
  Address start = Address::DeSerialize(fp);
  Address end = Address::DeSerialize(fp);
  std::string comment = Utility::StringDeSerialize(fp);
  return Range(start, end, comment);
}
