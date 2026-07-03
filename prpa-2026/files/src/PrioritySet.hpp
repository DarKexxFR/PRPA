#pragma once

#include <atomic>
#include <cstdint>
#include <vector>

class PrioritySetV1
{
public:
  static constexpr int NBITS  = 24;
  static constexpr int NWORDS = 1 << (NBITS - 6);

  PrioritySetV1();

  bool insert(int value);
  bool remove(int value);
  bool has(int value) const;
  int  get_min() const;
  int  pop_min();

private:
  std::vector<std::atomic<uint64_t>> m_bits;
};


class PrioritySet
{
public:
  static constexpr int NBITS   = 24;
  static constexpr int NLEVELS = NBITS / 6;

  PrioritySet();

  bool insert(int value);
  bool remove(int value);
  bool has(int value) const;
  int  get_min() const;
  int  pop_min();

private:
  std::vector<std::atomic<uint64_t>> m_bits[NLEVELS];
};
