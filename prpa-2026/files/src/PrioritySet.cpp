#include "PrioritySet.hpp"

#include <bit>

//
// Version 1: flat atomic bitset
//

PrioritySetV1::PrioritySetV1()
    : m_bits(NWORDS)
{
  for (auto& w : m_bits)
    w.store(0, std::memory_order_relaxed);
}

bool PrioritySetV1::insert(int value)
{
  uint64_t mask = uint64_t(1) << (value & 63);
  uint64_t old  = m_bits[value >> 6].fetch_or(mask);
  return !(old & mask);
}

bool PrioritySetV1::remove(int value)
{
  uint64_t mask = uint64_t(1) << (value & 63);
  uint64_t old  = m_bits[value >> 6].fetch_and(~mask);
  return (old & mask);
}

bool PrioritySetV1::has(int value) const
{
  uint64_t mask = uint64_t(1) << (value & 63);
  return m_bits[value >> 6].load() & mask;
}

int PrioritySetV1::get_min() const
{
  for (int w = 0; w < NWORDS; ++w)
  {
    uint64_t word = m_bits[w].load();
    if (word)
      return w * 64 + std::countr_zero(word);
  }
  return -1;
}

int PrioritySetV1::pop_min()
{
  // get_min then remove: if another thread stole the value in-between,
  // remove fails and we retry
  while (true)
  {
    int m = get_min();
    if (m == -1)
      return -1;
    if (remove(m))
      return m;
  }
}


//
// Version 2: hierarchical atomic bitset (k = 6, 4 levels)
//

PrioritySet::PrioritySet()
{
  int nwords = 1;
  for (int l = 0; l < NLEVELS; ++l)
  {
    m_bits[l] = std::vector<std::atomic<uint64_t>>(nwords);
    for (auto& w : m_bits[l])
      w.store(0, std::memory_order_relaxed);
    nwords *= 64;
  }
}

bool PrioritySet::insert(int value)
{
  // Set the leaf bit first (the leaf level is the source of truth)
  uint64_t mask = uint64_t(1) << (value & 63);
  uint64_t old  = m_bits[NLEVELS - 1][value >> 6].fetch_or(mask);

  if (old & mask)
    return false; // already present

  // Publish in the summary levels, bottom-up
  int idx = value >> 6;
  for (int l = NLEVELS - 2; l >= 0; --l)
  {
    uint64_t bit = uint64_t(1) << (idx & 63);
    idx >>= 6;
    uint64_t prev = m_bits[l][idx].fetch_or(bit);
    if (prev & bit)
      break; // the upper levels are already set
  }
  return true;
}

bool PrioritySet::remove(int value)
{
  uint64_t mask = uint64_t(1) << (value & 63);
  uint64_t old  = m_bits[NLEVELS - 1][value >> 6].fetch_and(~mask);

  if (!(old & mask))
    return false; // was absent

  // If we emptied the word, clear the summary bit... but a concurrent
  // insert may set a bit in the word right after our check. So after
  // clearing the summary bit we re-check the word and undo if needed.
  int idx = value >> 6;
  for (int l = NLEVELS - 2; l >= 0; --l)
  {
    if (m_bits[l + 1][idx].load() != 0)
      break; // word still in use, summaries must stay set

    uint64_t bit = uint64_t(1) << (idx & 63);
    int      up  = idx >> 6;
    m_bits[l][up].fetch_and(~bit);

    if (m_bits[l + 1][idx].load() != 0)
    {
      // a concurrent insert refilled the word: undo
      m_bits[l][up].fetch_or(bit);
      break;
    }
    idx = up;
  }
  return true;
}

bool PrioritySet::has(int value) const
{
  uint64_t mask = uint64_t(1) << (value & 63);
  return m_bits[NLEVELS - 1][value >> 6].load() & mask;
}

int PrioritySet::get_min() const
{
  // Walk down the hierarchy following the lowest set bit.
  // Summary bits are only hints: if we reach an empty word (concurrent
  // remove), we restart from the root.
  while (true)
  {
    int  idx   = 0;
    bool stale = false;

    for (int l = 0; l < NLEVELS; ++l)
    {
      uint64_t word = m_bits[l][idx].load();
      if (word == 0)
      {
        if (l == 0)
          return -1; // the set is really empty
        stale = true; // stale hint, retry
        break;
      }
      idx = idx * 64 + std::countr_zero(word);
    }

    if (!stale)
      return idx;
  }
}

int PrioritySet::pop_min()
{
  while (true)
  {
    int m = get_min();
    if (m == -1)
      return -1;
    if (remove(m)) // fails if another thread popped it first
      return m;
  }
}
