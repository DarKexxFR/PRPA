// Quick timing of PrioritySetV1 vs PrioritySet (no google-benchmark needed)
#include "PrioritySet.hpp"

#include <chrono>
#include <cstdio>
#include <random>
#include <thread>
#include <vector>

template <class Set>
static double mixed(int n_threads, int n_ops)
{
  Set s;
  auto t0 = std::chrono::high_resolution_clock::now();
  std::vector<std::thread> threads;
  for (int i = 0; i < n_threads; ++i)
    threads.emplace_back([&s, i, n_ops]() {
      std::minstd_rand gen(i);
      for (int j = 0; j < n_ops; ++j)
      {
        int v = gen() & ((1 << 24) - 1);
        switch (j % 4)
        {
        case 0: s.insert(v); break;
        case 1: s.remove(v); break;
        case 2: (void)s.has(v); break;
        case 3: (void)s.get_min(); break;
        }
      }
    });
  for (auto& t : threads)
    t.join();
  auto t1 = std::chrono::high_resolution_clock::now();
  return std::chrono::duration<double, std::milli>(t1 - t0).count();
}

int main()
{
  const int n_ops = 100000;
  std::printf("Mixed workload (%d ops/thread: 25%% insert, 25%% remove, 25%% has, 25%% get_min)\n", n_ops);
  std::printf("%-10s %12s %12s\n", "threads", "V1 flat", "V2 hier");
  for (int t : {1, 2, 4, 8})
  {
    double v1 = mixed<PrioritySetV1>(t, n_ops);
    double v2 = mixed<PrioritySet>(t, n_ops);
    std::printf("%-10d %9.1f ms %9.1f ms   (x%.0f)\n", t, v1, v2, v1 / v2);
  }
}
