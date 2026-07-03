#include "Otsu.hpp"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <random>

static Image<rgb8> make_test_image(int width, int height)
{
  Image<rgb8> img(width, height);
  std::minstd_rand gen(42);

  for (int y = 0; y < height; ++y)
  {
    rgb8* lineptr = (rgb8*)((std::byte*)img.buffer + y * img.stride);
    for (int x = 0; x < width; ++x)
    {
      int base   = (x / 64 + y / 64) % 2 ? 200 : 40;
      int noise  = (int)(gen() % 41) - 20;
      int v      = std::clamp(base + noise, 0, 255);
      lineptr[x] = {(uint8_t)v, (uint8_t)(gen() % 256), (uint8_t)(gen() % 256)};
    }
  }
  return img;
}

static bool same(const Image<rgb8>& a, const Image<rgb8>& b)
{
  for (int y = 0; y < a.height; ++y)
    if (std::memcmp((char*)a.buffer + y * a.stride, //
                    (char*)b.buffer + y * b.stride, //
                    a.width * sizeof(rgb8)) != 0)
      return false;
  return true;
}

int main()
{
  auto input = make_test_image(1920, 1080);

  auto ref = input.clone();
  otsu_baseline(ref);

  auto st = input.clone();
  otsu_st(st);

  auto mt = input.clone();
  otsu_mt(mt);

  int n_white = 0;
  for (int y = 0; y < ref.height; ++y)
  {
    rgb8* lineptr = (rgb8*)((std::byte*)ref.buffer + y * ref.stride);
    for (int x = 0; x < ref.width; ++x)
      n_white += lineptr[x].r == 255;
  }
  std::printf("white pixels: %d / %d (%.1f%%)\n", n_white, ref.width * ref.height,
              100.f * n_white / (ref.width * ref.height));

  bool ok_st = same(ref, st);
  bool ok_mt = same(ref, mt);
  std::printf("otsu_st == otsu_baseline: %s\n", ok_st ? "OK" : "FAIL");
  std::printf("otsu_mt == otsu_baseline: %s\n", ok_mt ? "OK" : "FAIL");

  return !(ok_st && ok_mt);
}
