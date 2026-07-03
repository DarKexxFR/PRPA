#include "Otsu.hpp"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <random>

static Image<rgb8> make(int w, int h)
{
  Image<rgb8> img(w, h);
  std::minstd_rand gen(42);
  for (int y = 0; y < h; ++y)
  {
    rgb8* p = (rgb8*)((std::byte*)img.buffer + y * img.stride);
    for (int x = 0; x < w; ++x)
    {
      int v = std::clamp(((x / 64 + y / 64) % 2 ? 200 : 40) + (int)(gen() % 41) - 20, 0, 255);
      p[x]  = {(uint8_t)v, (uint8_t)(gen() % 256), (uint8_t)(gen() % 256)};
    }
  }
  return img;
}

template <class F>
static double bench(F f, const Image<rgb8>& input, int iters)
{
  double best = 1e9;
  for (int i = 0; i < iters; ++i)
  {
    auto img = input.clone();
    auto t0  = std::chrono::high_resolution_clock::now();
    f(img);
    auto t1 = std::chrono::high_resolution_clock::now();
    best    = std::min(best, std::chrono::duration<double, std::milli>(t1 - t0).count());
  }
  return best;
}

int main()
{
  auto   input = make(1920, 1080);
  double t0    = bench(otsu_baseline, input, 20);
  double t1    = bench(otsu_st, input, 20);
  double t2    = bench(otsu_mt, input, 20);
  double mpix  = 1920.0 * 1080 / 1e6;

  std::printf("otsu_baseline: %7.2f ms  (%6.1f Mpix/s)\n", t0, mpix / t0 * 1000);
  std::printf("otsu_st:       %7.2f ms  (%6.1f Mpix/s)  x%.2f\n", t1, mpix / t1 * 1000, t0 / t1);
  std::printf("otsu_mt:       %7.2f ms  (%6.1f Mpix/s)  x%.2f\n", t2, mpix / t2 * 1000, t0 / t2);
}
