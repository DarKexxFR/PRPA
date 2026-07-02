#include "Otsu.hpp"

#include <benchmark/benchmark.h>
#include <functional>
#include <fmt/format.h>
#include "stb_image.h"


extern const char* ImagePath;
using otsu_fn_t = std::function<void(ImageView<rgb8>)>;

class BMScenario : public ::benchmark::Fixture
{
public:
  void SetUp(benchmark::State& )
    {
      if (!m_loaded)
      {
        m_input = Image<rgb8>(fmt::format("{}/test1.jpg", ImagePath));
      }
    }


  void process(benchmark::State& st, otsu_fn_t callback)
  {
      for (auto _ : st)
      {
        st.PauseTiming();
        auto input = m_input.clone();
        st.ResumeTiming();
        std::invoke(callback, input);
      }
      st.SetItemsProcessed(st.iterations() * m_input.width * m_input.height);
  }

protected:
  static bool           m_loaded;
  static Image<rgb8>    m_input;
};

bool        BMScenario::m_loaded = false;
Image<rgb8> BMScenario::m_input;

BENCHMARK_DEFINE_F(BMScenario, baseline)(benchmark::State& st) { this->process(st, &otsu_baseline); }
BENCHMARK_DEFINE_F(BMScenario, optimized)(benchmark::State& st) { this->process(st, &otsu_st); }
BENCHMARK_DEFINE_F(BMScenario, mt)(benchmark::State& st) { this->process(st, &otsu_mt); }

BENCHMARK_REGISTER_F(BMScenario, baseline)
    ->Unit(benchmark::kMillisecond) //
    ->UseRealTime();

BENCHMARK_REGISTER_F(BMScenario, optimized)
    ->Unit(benchmark::kMillisecond) //
    ->UseRealTime();

BENCHMARK_REGISTER_F(BMScenario, mt)
    ->Unit(benchmark::kMillisecond) //
    ->UseRealTime();

BENCHMARK_MAIN();
