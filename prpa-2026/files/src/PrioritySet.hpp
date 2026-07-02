#pragma once

#include <atomic>
#include <cstdint>
#include <vector>

// Concurrent set of 24-bits integers (the PES problem)
//
// bool insert(int value);  true if inserted (false if already present)
// bool remove(int value);  true if removed (false if absent)
// bool has(int value);     true if present
// int  get_min();          smallest element (-1 if empty)
// int  pop_min();          remove and return the smallest element (-1 if empty)


// Version 1: flat atomic bitset.
// - insert/remove/has are a single atomic RMW on one 64-bits word => lock-free
// - get_min is a linear scan over the 2^18 words (slow but correct)
// Used as a reference implementation for tests and benchmarks.
class PrioritySetV1
{
public:
  static constexpr int NBITS  = 24;
  static constexpr int NWORDS = 1 << (NBITS - 6); // 262144 words = 2 MiB

  PrioritySetV1();

  bool insert(int value);
  bool remove(int value);
  bool has(int value) const;
  int  get_min() const;
  int  pop_min();

private:
  std::vector<std::atomic<uint64_t>> m_bits;
};


// Version 2 (final): hierarchical atomic bitset.
//
// The values are stored in the leaf bitset (level 3, 2^24 bits).
// Each upper level summarizes the level below: bit i of level L is set
// when the word i of level L+1 is (probably) non-empty.
//
// With k = 6 (buckets of 2^6 = 64 bits = one uint64_t, the widest type
// with cheap atomic RMW on x86-64):
//   level 0:       1 word  (     64 bits)
//   level 1:      64 words (   4096 bits)
//   level 2:    4096 words ( 262144 bits)
//   level 3:  262144 words (2^24 bits, the values)
//
// get_min walks down the hierarchy following the lowest set bit of each
// level (std::countr_zero) => 4 loads instead of a 262144-words scan.
//
// Concurrency: everything is done with fetch_or / fetch_and (lock-free,
// no mutex). The summary levels are just hints:
// - insert sets the leaf bit then ALWAYS sets the 3 summary bits
// - remove clears the leaf bit; if the word becomes empty it clears the
//   summary bit, then re-checks the word: if a concurrent insert refilled
//   it in-between, the bit is set back (re-check & undo pattern)
// - get_min restarts from the root when it reaches a word that a
//   concurrent remove has emptied (stale hint)
class PrioritySet
{
public:
  static constexpr int NBITS   = 24;
  static constexpr int NLEVELS = NBITS / 6; // k = 6 => 4 levels

  PrioritySet();

  bool insert(int value);
  bool remove(int value);
  bool has(int value) const;
  int  get_min() const;
  int  pop_min();

private:
  // m_bits[l][w] = word w of level l (level NLEVELS-1 = leaves)
  std::vector<std::atomic<uint64_t>> m_bits[NLEVELS];
};
