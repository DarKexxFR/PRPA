#include "Otsu.hpp"

#include <cstring>
#include <oneapi/tbb.h>

// Compute the otsu threshold from a 256-bins histogram (naive version)
// For each candidate threshold we re-scan the whole histogram.
static int otsu_threshold_naive(const int hist[256], int total)
{
  double best_var  = 0.0;
  int    threshold = 0;

  for (int t = 0; t < 256; ++t)
  {
    long w0 = 0, w1 = 0;
    double sum0 = 0, sum1 = 0;

    for (int i = 0; i <= t; ++i)
    {
      w0   += hist[i];
      sum0 += i * hist[i];
    }
    for (int i = t + 1; i < 256; ++i)
    {
      w1   += hist[i];
      sum1 += i * hist[i];
    }

    if (w0 == 0 || w1 == 0)
      continue;

    double mu0 = sum0 / w0;
    double mu1 = sum1 / w1;
    double var = (double)w0 / total * (double)w1 / total * (mu0 - mu1) * (mu0 - mu1);

    if (var > best_var)
    {
      best_var  = var;
      threshold = t;
    }
  }
  return threshold;
}

// Same result but O(256): the sums are computed incrementally
static int otsu_threshold(const int hist[256], int total)
{
  long sum_all = 0;
  for (int i = 0; i < 256; ++i)
    sum_all += (long)i * hist[i];

  double best_var  = 0.0;
  int    threshold = 0;
  long   w0 = 0, sum0 = 0;

  for (int t = 0; t < 256; ++t)
  {
    w0   += hist[t];
    sum0 += (long)t * hist[t];
    long w1 = total - w0;

    if (w0 == 0 || w1 == 0)
      continue;

    double mu0 = (double)sum0 / w0;
    double mu1 = (double)(sum_all - sum0) / w1;
    double var = (double)w0 * w1 * (mu0 - mu1) * (mu0 - mu1);

    if (var > best_var)
    {
      best_var  = var;
      threshold = t;
    }
  }
  return threshold;
}

// Single threaded, naive baseline implementation
void otsu_baseline(ImageView<rgb8> in)
{
  int hist[256] = {0};

  // 1. Grayscale histogram (float math, gray recomputed later)
  for (int y = 0; y < in.height; ++y)
  {
    rgb8* lineptr = (rgb8*)((std::byte*)in.buffer + y * in.stride);
    for (int x = 0; x < in.width; ++x)
    {
      float gray = (lineptr[x].r + lineptr[x].g + lineptr[x].b) / 3.f;
      hist[(int)gray]++;
    }
  }

  // 2. Find the threshold that maximizes the inter-class variance
  int t = otsu_threshold_naive(hist, in.width * in.height);

  // 3. Binarize (gray is recomputed a second time)
  for (int y = 0; y < in.height; ++y)
  {
    rgb8* lineptr = (rgb8*)((std::byte*)in.buffer + y * in.stride);
    for (int x = 0; x < in.width; ++x)
    {
      float   gray = (lineptr[x].r + lineptr[x].g + lineptr[x].b) / 3.f;
      uint8_t v    = (int)gray > t ? 255 : 0;
      lineptr[x]   = {v, v, v};
    }
  }
}


// Single threaded - Optimized version of the Method
// - integer arithmetic only
// - the gray value is cached in the red channel during the first pass
//   (halves the memory traffic of the second pass + vectorizable loops)
// - O(256) incremental threshold computation
void otsu_st(ImageView<rgb8> in)
{
  int hist[256] = {0};

  for (int y = 0; y < in.height; ++y)
  {
    rgb8* lineptr = (rgb8*)((std::byte*)in.buffer + y * in.stride);
    for (int x = 0; x < in.width; ++x)
    {
      int gray      = (lineptr[x].r + lineptr[x].g + lineptr[x].b) / 3;
      lineptr[x].r  = (uint8_t)gray; // cache it for the 2nd pass
      hist[gray]++;
    }
  }

  int t = otsu_threshold(hist, in.width * in.height);

  for (int y = 0; y < in.height; ++y)
  {
    rgb8* lineptr = (rgb8*)((std::byte*)in.buffer + y * in.stride);
    for (int x = 0; x < in.width; ++x)
    {
      uint8_t v  = lineptr[x].r > t ? 255 : 0;
      lineptr[x] = {v, v, v};
    }
  }
}


// Map + Reduction pattern: each body computes a local histogram on its
// sub-range (and caches the gray value), histograms are merged in join()
class HistogramComputer
{
public:
  ImageView<rgb8> in;
  int hist[256] = {0};

  HistogramComputer(ImageView<rgb8> img)
    : in{img}
  {
  }

  HistogramComputer(HistogramComputer& other, tbb::split)
    : in{other.in}
  {
  }

  void operator()(const tbb::blocked_range2d<int>& r)
  {
    auto rows = r.rows();
    auto cols = r.cols();

    for (int y = rows.begin(); y < rows.end(); y++)
    {
      rgb8* lineptr = (rgb8*)((std::byte*)in.buffer + y * in.stride);
      for (int x = cols.begin(); x < cols.end(); x++)
      {
        int gray     = (lineptr[x].r + lineptr[x].g + lineptr[x].b) / 3;
        lineptr[x].r = (uint8_t)gray;
        hist[gray]++;
      }
    }
  }

  void join(const HistogramComputer& other)
  {
    for (int i = 0; i < 256; ++i)
      hist[i] += other.hist[i];
  }
};

// Element-wise pattern: pixels are independent
class BinarizeApplier
{
public:
  ImageView<rgb8> in;
  int threshold;

  void operator()(const tbb::blocked_range2d<int>& r) const
  {
    auto rows = r.rows();
    auto cols = r.cols();

    for (int y = rows.begin(); y < rows.end(); y++)
    {
      rgb8* lineptr = (rgb8*)((std::byte*)in.buffer + y * in.stride);
      for (int x = cols.begin(); x < cols.end(); x++)
      {
        uint8_t v  = lineptr[x].r > threshold ? 255 : 0;
        lineptr[x] = {v, v, v};
      }
    }
  }
};

// Multi threaded version of the Method
void otsu_mt(ImageView<rgb8> in)
{
  int grain_size = 16;
  auto rng = tbb::blocked_range2d<int>(0, in.height, grain_size, 0, in.width, in.width);

  HistogramComputer histo{in};
  tbb::parallel_reduce(rng, histo);

  int t = otsu_threshold(histo.hist, in.width * in.height);

  BinarizeApplier algo = {
    .in        = in,
    .threshold = t
  };
  tbb::parallel_for(rng, algo);
}

extern "C" {

static Method m_method = BASELINE;

void set_otsu_method(Method m)
{
  m_method = m;
}


void otsu(uint8_t* buffer, int width, int height, int stride)
{
  if (buffer)
  {
    switch (m_method)
    {
    case BASELINE:
      otsu_baseline({(rgb8*)buffer, width, height, stride});
      break;
    case ST:
      otsu_st({(rgb8*)buffer, width, height, stride});
      break;
    case MT:
      otsu_mt({(rgb8*)buffer, width, height, stride});
      break;
    }
  }
}


}
