#include "PrioritySet.hpp"

#include <algorithm>
#include <atomic>
#include <cassert>
#include <cstdio>
#include <set>
#include <thread>
#include <vector>

static int n_failed = 0;

#ifdef __SANITIZE_THREAD__
static constexpr int STRESS = 10;
#else
static constexpr int STRESS = 1;
#endif

#define CHECK(cond)                                                                                                    \
  do                                                                                                                   \
  {                                                                                                                    \
    if (!(cond))                                                                                                       \
    {                                                                                                                  \
      std::printf("  FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);                                                    \
      n_failed++;                                                                                                      \
    }                                                                                                                  \
  } while (0)


template <class Set>
void test_sequential()
{
  Set s;
  CHECK(s.get_min() == -1);
  CHECK(s.pop_min() == -1);

  CHECK(s.insert(42));
  CHECK(!s.insert(42)); // already present
  CHECK(s.has(42));
  CHECK(!s.has(41));

  CHECK(s.insert(7));
  CHECK(s.insert((1 << 24) - 1)); // max 24-bits value
  CHECK(s.insert(0));

  CHECK(s.get_min() == 0);
  CHECK(s.pop_min() == 0);
  CHECK(s.pop_min() == 7);
  CHECK(s.pop_min() == 42);
  CHECK(s.pop_min() == (1 << 24) - 1);
  CHECK(s.pop_min() == -1);

  CHECK(!s.remove(42)); // already removed
}

template <class Set>
void test_concurrent_inserts()
{
  Set s;
  std::vector<std::thread> threads;
  for (int i = 0; i < 10; ++i)
    threads.emplace_back([&s, i]() { s.insert(i); });
  for (auto& t : threads)
    t.join();

  for (int i = 0; i < 10; ++i)
    CHECK(s.has(i));
  CHECK(s.get_min() == 0);
}

template <class Set>
void test_insert_remove_conflict()
{
  const int k     = 8;
  const int iters = 10000 / STRESS;

  for (int run = 0; run < 20; ++run)
  {
    Set s;
    std::atomic<int> n = 0;

    std::vector<std::thread> threads;
    for (int i = 0; i < k; ++i)
      threads.emplace_back([&]() {
        for (int j = 0; j < iters; ++j)
          n += s.insert(99);
      });
    for (int i = 0; i < k; ++i)
      threads.emplace_back([&]() {
        for (int j = 0; j < iters; ++j)
          n -= s.remove(99);
      });
    for (auto& t : threads)
      t.join();

    CHECK(n >= 0);
    CHECK(n <= 1); // one single value: at most 1 more insert than removes
    if (n == 0)
      CHECK(!s.has(99));
    else
      CHECK(s.has(99));
  }
}

template <class Set>
void test_disjoint_inserts()
{
  const int k       = 8;
  const int per_thr = 50000 / STRESS;

  Set s;
  std::vector<std::thread> threads;
  for (int i = 0; i < k; ++i)
    threads.emplace_back([&s, i, per_thr]() {
      for (int v = i * per_thr; v < (i + 1) * per_thr; ++v)
        s.insert(v);
    });
  for (auto& t : threads)
    t.join();

  bool all = true;
  for (int v = 0; v < k * per_thr; ++v)
    all &= s.has(v);
  CHECK(all);
  CHECK(s.get_min() == 0);
}

template <class Set>
void test_concurrent_pop_min()
{
  const int n_values = 100000 / STRESS;
  const int k        = 8;

  Set s;
  for (int v = 0; v < n_values; ++v)
    s.insert(v);

  std::vector<std::vector<int>> popped(k);
  std::vector<std::thread> threads;
  for (int i = 0; i < k; ++i)
    threads.emplace_back([&s, &popped, i]() {
      while (true)
      {
        int v = s.pop_min();
        if (v == -1)
          break;
        popped[i].push_back(v);
      }
    });
  for (auto& t : threads)
    t.join();

  std::set<int> all;
  std::size_t   total = 0;
  for (int i = 0; i < k; ++i)
  {
    total += popped[i].size();
    all.insert(popped[i].begin(), popped[i].end());

    bool sorted = std::is_sorted(popped[i].begin(), popped[i].end());
    CHECK(sorted);
  }
  CHECK(total == n_values);
  CHECK(all.size() == n_values);
  CHECK(s.get_min() == -1);
}

template <class Set>
void test_producer_consumer()
{
  const int n_prod   = 4;
  const int n_cons   = 4;
  const int per_prod = 25000 / STRESS;

  Set s;
  std::atomic<int>  produced = 0;
  std::atomic<long> pop_count = 0;

  std::vector<std::thread> threads;
  for (int i = 0; i < n_prod; ++i)
    threads.emplace_back([&, i]() {
      for (int j = 0; j < per_prod; ++j)
      {
        s.insert(i * per_prod + j);
        produced++;
      }
    });
  for (int i = 0; i < n_cons; ++i)
    threads.emplace_back([&]() {
      while (produced.load() < n_prod * per_prod || s.get_min() != -1)
      {
        int v = s.pop_min();
        if (v != -1)
          pop_count++;
      }
    });
  for (auto& t : threads)
    t.join();

  CHECK(pop_count == n_prod * per_prod);
  CHECK(s.get_min() == -1);
}


template <class Set>
void run_all(const char* name)
{
  std::printf("== %s ==\n", name);
  std::printf("test_sequential...\n");
  test_sequential<Set>();
  std::printf("test_concurrent_inserts...\n");
  test_concurrent_inserts<Set>();
  std::printf("test_insert_remove_conflict...\n");
  test_insert_remove_conflict<Set>();
  std::printf("test_disjoint_inserts...\n");
  test_disjoint_inserts<Set>();
  std::printf("test_concurrent_pop_min...\n");
  test_concurrent_pop_min<Set>();
  std::printf("test_producer_consumer...\n");
  test_producer_consumer<Set>();
}

int main()
{
  run_all<PrioritySetV1>("PrioritySetV1 (flat bitset)");
  run_all<PrioritySet>("PrioritySet (hierarchical bitset)");

  if (n_failed == 0)
    std::printf("ALL TESTS PASSED\n");
  else
    std::printf("%d CHECKS FAILED\n", n_failed);
  return n_failed != 0;
}
