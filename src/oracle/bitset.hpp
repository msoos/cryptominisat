// SharpSAT-TD is a modification of SharpSAT (MIT License, 2019 Marc Thurley).
//
// SharpSAT-TD -- Copyright (c) 2021 Tuukka Korhonen
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
// DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
// OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE
// OR OTHER DEALINGS IN THE SOFTWARE.

#pragma once

#include <cstdint>
#include <cstdlib>
#include <vector>
#include <cstring>
#include <random>
#include <limits>
#include <cassert>
#include <functional>

#define BITS 64

namespace sspp {
class Bitset {
 public:
  uint64_t* data_;
  size_t chunks_;
  Bitset() {
    chunks_ = 0;
    data_ = nullptr;
  }
  explicit Bitset(size_t size) {
    chunks_ = (size + BITS - 1) / BITS;
    data_ = (uint64_t*)std::malloc(chunks_*sizeof(uint64_t));
    for (size_t i=0;i<chunks_;i++){
      data_[i] = 0;
    }
  }
  ~Bitset() {
    std::free(data_);
  }
  Bitset(const Bitset& other) {
    chunks_ = other.chunks_;
    data_ = (uint64_t*)std::malloc(chunks_*sizeof(uint64_t));
    for (size_t i=0;i<chunks_;i++){
      data_[i] = other.data_[i];
    }
  }
  Bitset& operator=(const Bitset& other) {
    if (this != &other) {
      if (chunks_ != other.chunks_) {
        std::free(data_);
        chunks_ = other.chunks_;
        data_ = (uint64_t*)std::malloc(chunks_*sizeof(uint64_t));
      }
      for (size_t i=0;i<chunks_;i++){
        data_[i] = other.data_[i];
      }
    }
    return *this;
  }
  Bitset(Bitset&& other) {
    data_ = other.data_;
    chunks_ = other.chunks_;
    other.data_ = nullptr;
    other.chunks_ = 0;
  }
  Bitset& operator=(Bitset&& other) {
    if (this != &other) {
      std::free(data_);
      data_ = other.data_;
      chunks_ = other.chunks_;
      other.data_ = nullptr;
      other.chunks_ = 0;
    }
    return *this;
  }
  bool operator<(const Bitset& other) const {
    for (size_t i=0;i<chunks_;i++){
      if (data_[i]<other.data_[i]) return true;
      else if(data_[i]>other.data_[i]) return false;
    }
    return false;
  }
  bool operator==(const Bitset& other) const {
    for (size_t i=0;i<chunks_;i++){
      if (data_[i] != other.data_[i]) return false;
    }
    return true;
  }
  bool operator!=(const Bitset& other) const {
    for (size_t i=0;i<chunks_;i++){
      if (data_[i] != other.data_[i]) return true;
    }
    return false;
  }
  Bitset operator|(const Bitset& other) const {
    Bitset ret(BITS*chunks_);
    for (size_t i=0;i<chunks_;i++){
      ret.data_[i] = data_[i] | other.data_[i];
    }
    return ret;
  }
  Bitset operator&(const Bitset& other) const {
    Bitset ret(BITS*chunks_);
    for (size_t i=0;i<chunks_;i++){
      ret.data_[i] = data_[i] & other.data_[i];
    }
    return ret;
  }
  Bitset operator~() const {
    Bitset ret(BITS*chunks_);
    for (size_t i=0;i<chunks_;i++){
      ret.data_[i] = (~data_[i]);
    }
    return ret;
  }
  void CopyFrom(const Bitset& other) {
    for (size_t i=0;i<chunks_;i++){
      data_[i] = other.data_[i];
    }
  }
  void Set(size_t i, bool v) {
    if (v) {
      data_[i/BITS] |= ((uint64_t)1 << (uint64_t)(i%BITS));
    } else {
      data_[i/BITS] &= (~((uint64_t)1 << (uint64_t)(i%BITS)));
    }
  }
  void SetTrue(size_t i) {
    data_[i/BITS] |= ((uint64_t)1 << (uint64_t)(i%BITS));
  }
  void SetFalse(size_t i) {
    data_[i/BITS] &= (~((uint64_t)1 << (uint64_t)(i%BITS)));
  }
  void SetTrue(const std::vector<size_t>& v) {
    for (size_t x : v) {
      SetTrue(x);
    }
  }
  void SetTrue(const std::vector<int>& v) {
    for (int x : v) {
      SetTrue(x);
    }
  }
  void SetFalse(const std::vector<int>& v) {
    for (int x : v) {
      SetFalse(x);
    }
  }
  void FillTrue() {
    for (size_t i=0;i<chunks_;i++){
      data_[i] = ~0;
    }
  }
  void FillUpTo(size_t n) {
    for (size_t i=0;i<chunks_;i++){
      if ((i+1)*BITS <= n) {
        data_[i] = ~0;
      } else if (i*BITS < n) {
        for (size_t j=i*BITS;j<n;j++){
          SetTrue(j);
        }
      } else {
        return;
      }
    }
  }
  bool Get(size_t i) const {
    return data_[i/BITS] & ((uint64_t)1 << (uint64_t)(i%BITS));
  }
  void Clear() {
    for (size_t i=0;i<chunks_;i++){
      data_[i] = 0;
    }
  }
  bool IsEmpty() const {
    for (size_t i=0;i<chunks_;i++){
      if (data_[i]) return false;
    }
    return true;
  }
  uint64_t Chunk(size_t i) const {
    return data_[i];
  }
  uint64_t& Chunk(size_t i) {
    return data_[i];
  }
  size_t Chunks() const {
    return chunks_;
  }
  void operator |= (const Bitset& rhs) {
    for (size_t i=0;i<chunks_;i++){
      data_[i] |= rhs.data_[i];
    }
  }
  void operator &= (const Bitset& rhs) {
    for (size_t i=0;i<chunks_;i++){
      data_[i] &= rhs.data_[i];
    }
  }
  void TurnOff(const Bitset& rhs) {
    for (size_t i=0;i<chunks_;i++){
      data_[i] &= (~rhs.data_[i]);
    }
  }
  void InvertAnd(const Bitset& rhs) {
    for (size_t i=0;i<chunks_;i++){
      data_[i] = (~data_[i]) & rhs.data_[i];
    }
  }
  void SetNeg(const Bitset& rhs) {
    for (size_t i=0;i<chunks_;i++){
      data_[i] = ~rhs.data_[i];
    }
  }
  void SetNegAnd(const Bitset& rhs1, const Bitset& rhs2) {
    for (size_t i=0;i<chunks_;i++){
      data_[i] = (~rhs1.data_[i]) & rhs2.data_[i];
    }
  }
  void SetAnd(const Bitset& rhs1, const Bitset& rhs2) {
    for (size_t i=0;i<chunks_;i++){
      data_[i] = rhs1.data_[i] & rhs2.data_[i];
    }
  }
  bool Subsumes(const Bitset& other) const {
    for (size_t i=0;i<chunks_;i++){
      if ((data_[i] | other.data_[i]) != data_[i]) return false;
    }
    return true;
  }
  std::vector<int> Elements() const {
    std::vector<int> ret;
    for (size_t i=0;i<chunks_;i++){
      uint64_t td = data_[i];
      while (td) {
        ret.push_back(i*BITS + __builtin_ctzll(td));
        td &= ~-td;
      }
    }
    return ret;
  }
  int Popcount() const {
    int cnt = 0;
    for (size_t i=0;i<chunks_;i++) {
      cnt += __builtin_popcountll(data_[i]);
    }
    return cnt;
  }
  bool Intersects(const Bitset& other) const {
    for (size_t i=0;i<chunks_;i++){
      if (data_[i] & other.data_[i]) return true;
    }
    return false;
  }
  int IntersectionPopcount(const Bitset& other) const {
    int cnt = 0;
    for (size_t i=0;i<chunks_;i++) {
      cnt += __builtin_popcountll(data_[i] & other.data_[i]);
    }
    return cnt;
  }
  int First() const {
    for (size_t i=0;i<chunks_;i++) {
      if (data_[i]) {
        return i*BITS + __builtin_ctzll(data_[i]);
      }
    }
    return chunks_ * BITS;
  }

  class BitsetIterator {
   private:
    const Bitset* const bitset_;
    size_t pos_;
    uint64_t tb_;
   public:
    BitsetIterator(const Bitset* const bitset, size_t pos, uint64_t tb) : bitset_(bitset), pos_(pos), tb_(tb) { }
    bool operator!=(const BitsetIterator& other) const {
      return pos_ != other.pos_ || tb_ != other.tb_;
    }
    const BitsetIterator& operator++() {
      tb_ &= ~-tb_;
      while (tb_ == 0 && pos_ < bitset_->chunks_) {
        pos_++;
        if (pos_ < bitset_->chunks_) {
          tb_ = bitset_->data_[pos_];
        }
      }
      return *this;
    }
    int operator*() const {
      return pos_*BITS + __builtin_ctzll(tb_);
    }
  };

  BitsetIterator begin() const {
    size_t pos = 0;
    while (pos < chunks_ && data_[pos] == 0) {
      pos++;
    }
    if (pos < chunks_) {
      return BitsetIterator(this, pos, data_[pos]);
    } else {
      return BitsetIterator(this, pos, 0);
    }
  }
  BitsetIterator end() const {
    return BitsetIterator(this, chunks_, 0);
  }
};
} // namespace sspp
