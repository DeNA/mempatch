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

#include <assert.h>
#include <set>
#include <stdlib.h>
#include <string>

#include "ChangeString.h"

class Range;
class Address {
public:
  Address() : addr_(0) { ; }
  explicit Address(size_t addr) : addr_(addr) {}

  size_t to_i() const { return addr_; }
  void Add(size_t rhs) { addr_ += rhs; }
  size_t Dist(const Address &rhs) const { return std::min(addr_ - rhs.addr_, rhs.addr_ - addr_); }

  bool operator<(const Address &a) const { return addr_ < a.addr_; }
  bool operator>(const Address &a) const { return addr_ > a.addr_; }
  bool operator<=(const Address &a) const { return addr_ <= a.addr_; }
  bool operator>=(const Address &a) const { return addr_ >= a.addr_; }
  bool operator==(const Address &a) const { return addr_ == a.addr_; }
  bool operator!=(const Address &a) const { return addr_ != a.addr_; }

  void Serialize(FILE *fp) const;
  static Address DeSerialize(FILE *fp);
  const std::string GetComment(const std::set<Range> &range_set) const;

private:
  size_t addr_;
};

// [start, end)
class Range {
public:
  Range() { ; }
  Range(const Address &start, const Address &end) : start_(start), end_(end) { assert(end >= start); }
  Range(const Address &start, const Address &end, const std::string &comment)
      : start_(start), end_(end), comment_(comment) {
    assert(end >= start);
  }
  Range(const size_t start, const size_t end, const std::string &comment)
      : start_(start), end_(end), comment_(comment) {
    assert(end >= start);
  }
  const Address &GetStart() const { return start_; }
  const Address &GetEnd() const { return end_; }
  size_t Size() const { return end_.to_i() - start_.to_i(); }
  const std::string &GetComment() const { return comment_; }

  bool operator<(const Range &rhs) const {
    if (start_ < rhs.start_ || rhs.start_ < start_) {
      return start_ < rhs.start_;
    }
    return end_ < rhs.end_;
  }

  bool IsSuperset(const Range &sub) const {
    return start_.to_i() <= sub.GetStart().to_i() && sub.GetEnd().to_i() <= end_.to_i();
  }
  bool isSubset(const Range &super) const { return super.IsSuperset(*this); }

  // addressやrangeがrange_setの内部に入る様にする
  // range_setの外にある場合はaddressを0にする
  static Address Fit(const std::set<Range> &range_set, const Address &address);
  static Range Fit(const std::set<Range> &range_set, const Range &range);

  void Serialize(FILE *fp) const;
  static Range DeSerialize(FILE *fp);

private:
  Address start_;
  Address end_;
  std::string comment_; // const指定するとデフォルトのコピーコンストラクタが使えなくなる
};
typedef std::set<Range> RangeSet;

class TargetAddress {
public:
  TargetAddress() { ; }
  TargetAddress(const Address &addr, const ChangeString &changestr) : addr_(addr), changestr_(changestr) { ; }
  const Address &GetAddress() const { return addr_; }
  const ChangeString &GetChangeString() const { return changestr_; }
  bool operator<(const TargetAddress &rhs) const {
    if (addr_ < rhs.addr_ || rhs.addr_ < addr_) {
      return addr_ < rhs.addr_;
    }
    return changestr_ < rhs.changestr_;
  }

  void Serialize(FILE *fp) const {
    addr_.Serialize(fp);
    changestr_.Serialize(fp);
  }
  static TargetAddress DeSerialize(FILE *fp) {
    Address addr = Address::DeSerialize(fp);
    ChangeString changestr = ChangeString::DeSerialize(fp);
    return TargetAddress(addr, changestr);
  }

private:
  Address addr_;
  ChangeString changestr_;
};
