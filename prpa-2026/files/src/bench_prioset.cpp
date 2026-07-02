// Benchmarks for the PrioritySet: flat bitset (V1) vs hierarchical (V2)

#include "PrioritySet.hpp"

#include <benchmark/benchmark.h>
#include <random>
#include <thread>
#include <vector>

// Mixed workload: each thread does inserts / removes / get_min on random
// values. The number of threads is the benchmark argument.
template <class Set>
static void BM_MixedWorkload(benchmark::State& st)
{
  const int n_threads = st.range(0);
  const int n_ops     = 100000;

  for (auto _ : st)
  {
    Set s;
    std::vector<std::thread> threads;
    for (int i = 0; i < n_threads; ++i)
      threads.emplace_back([&s, i]() {
        std::minstd_rand gen(i);
        for (int j = 0; j < n_ops; ++j)
        {
          int v = gen() & ((1 << 24) - 1);
          switch (j % 4)
          {
          case 0: s.insert(v); break;
          case 1: s.remove(v); break;
          case 2: benchmark::DoNotOptimize(s.has(v)); break;
          case 3: benchmark::DoNotOptimize(s.get_min()); break;
          }
        }
      });
    for (auto& t : threads)
      t.join();
  }
  st.SetItemsProcessed(st.iterations() * n_threads * n_ops);
}

BENCHMARK_TEMPLATE(BM_MixedWorkload, PrioritySetV1)
    ->Arg(1)->Arg(2)->Arg(4)->Arg(8)
    ->Unit(benchmark::kMillisecond)
    ->UseRealTime();

BENCHMARK_TEMPLATE(BM_MixedWorkload, PrioritySet)
    ->Arg(1)->Arg(2)->Arg(4)->Arg(8)
    ->Unit(benchmark::kMillisecond)
    ->UseRealTime();


// get_min-heavy workload: this is where the hierarchy pays off
template <class Set>
static void BM_GetMin(benchmark::State& st)
{
  Set s;
  std::minstd_rand gen(42);
  for (int i = 0; i < 1000; ++i)
    s.insert((8 << 20) + (gen() & ((1 << 20) - 1))); // large values = long scan for V1

  for (auto _ : st)
    benchmark::DoNotOptimize(s.get_min());

  st.SetItemsProcessed(st.iterations());
}

BENCHMARK_TEMPLATE(BM_GetMin, PrioritySetV1);
BENCHMARK_TEMPLATE(BM_GetMin, PrioritySet);

BENCHMARK_MAIN();
