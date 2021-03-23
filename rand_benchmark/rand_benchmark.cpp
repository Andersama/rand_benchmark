// rand_benchmark.cpp : Defines the entry point for the application.
//

#define ANKERL_NANOBENCH_IMPLEMENT
#include "nanobench.h"

#include <algorithm>
#include <chrono>
#include <random>
#include <vector>

typedef struct {
  uint64_t state;
  uint64_t inc;
} pcg32_random_t;

uint32_t pcg32_random_r(pcg32_random_t *rng) {
  uint64_t oldstate = rng->state;
  // Advance internal state
  rng->state = oldstate * 6364136223846793005ULL + (rng->inc | 1);
  // Calculate output function (XSH RR), uses old state for max ILP
  uint32_t xorshifted = ((oldstate >> 18u) ^ oldstate) >> 27u;
  uint32_t rot = oldstate >> 59u;
  return (xorshifted >> rot) | (xorshifted << ((-rot) & 31));
}

struct romu192_random_t {
  uint64_t xstate;
  uint64_t ystate;
  uint64_t zstate;
};

constexpr uint64_t romu64_random_r(romu192_random_t *rng) {
  //#define ROMU_ROTL(d, lrot) ((d << (lrot)) | (d >> (8 * sizeof(d) - (lrot))))
  uint64_t xp = rng->xstate;
  uint64_t yp = rng->ystate;
  uint64_t zp = rng->zstate;
  rng->xstate = 15241094284759029579u * zp;
  rng->ystate = yp - xp;
  rng->ystate = ((rng->ystate << (12)) |
                 (rng->ystate >> (8 * sizeof(rng->ystate) - (12))));
  rng->zstate = zp - yp;
  rng->zstate = ((rng->zstate << (44)) |
                 (rng->zstate >> (8 * sizeof(rng->zstate) - (44))));
  return xp;
}

constexpr uint64_t romu64_random_edit_r(romu192_random_t *rng) {
  //#define ROMU_ROTL(d, lrot) ((d << (lrot)) | (d >> (8 * sizeof(d) - (lrot))))
  uint64_t xp = rng->xstate;
  uint64_t yp = rng->ystate;
  uint64_t zp = rng->zstate;
  rng->xstate = 15241094284759029579u * zp + 1ULL;
  rng->ystate = yp - xp;
  rng->ystate = ((rng->ystate << (12)) |
                 (rng->ystate >> (8 * sizeof(rng->ystate) - (12))));
  rng->zstate = zp - yp;
  rng->zstate = ((rng->zstate << (44)) |
                 (rng->zstate >> (8 * sizeof(rng->zstate) - (44))));
  return xp;
}

constexpr uint64_t romu64_random_edit2_r(romu192_random_t *rng) {
  //#define ROMU_ROTL(d, lrot) ((d << (lrot)) | (d >> (8 * sizeof(d) - (lrot))))
  uint64_t xp = rng->xstate;
  uint64_t yp = rng->ystate;
  uint64_t zp = rng->zstate;
  rng->xstate = 15241094284759029579u * zp + (yp|1ULL);
  rng->ystate = yp - xp;
  rng->ystate = ((rng->ystate << (12)) |
                 (rng->ystate >> (8 * sizeof(rng->ystate) - (12))));
  rng->zstate = zp - yp;
  rng->zstate = ((rng->zstate << (44)) |
                 (rng->zstate >> (8 * sizeof(rng->zstate) - (44))));
  return xp;
}

int main() {
  std::chrono::high_resolution_clock clk;

  romu192_random_t romu_rng;
  pcg32_random_t rng;
  rng.inc = std::chrono::duration_cast<std::chrono::nanoseconds>(
                clk.now().time_since_epoch())
                .count();
  romu_rng.xstate = std::chrono::duration_cast<std::chrono::nanoseconds>(
                        clk.now().time_since_epoch())
                        .count();

  std::random_device dev;
  std::mt19937_64 mt_rng(dev());
  ankerl::nanobench::Rng ank_rng(dev());

  ankerl::nanobench::Bench benchmark;
  // benchmark.epochs(1024);
  // benchmark.minEpochIterations(128);
  // benchmark.minEpochTime(std::chrono::nanoseconds{4000});
  benchmark.warmup(4);
  // for metrics
  // benchmark.unit("sorts");
  benchmark.performanceCounters(true);
  benchmark.relative(true);

  size_t bench_size = 1024;
  size_t target_size = 50000;
  std::vector<uint32_t> data;

  data.reserve(target_size);
  // flip the bits since the patterns probably similar to inc
  rng.state = ~(std::chrono::duration_cast<std::chrono::nanoseconds>(
                    clk.now().time_since_epoch())
                    .count());
  romu_rng.ystate = ~(std::chrono::duration_cast<std::chrono::nanoseconds>(
                        clk.now().time_since_epoch())
                        .count());
  std::string title = std::string{"________________________________"};
  while (bench_size < target_size) {
    benchmark.batch(bench_size);
    while (data.size() < bench_size) {
      data.emplace_back();
    }
    for (size_t i = 0; i < data.size(); i++) {
      data[i] = pcg32_random_r(&rng);
    }

    std::string bench_num = std::to_string(bench_size);

    title = std::string{"romu ("} + bench_num + ")";
    benchmark.run(title, [&]() {
      for (size_t i = 0; i < data.size(); i++) {
        data[i] = romu64_random_r(&romu_rng);
      }
    });

    title = std::string{"romu edit ("} + bench_num + ")";
    benchmark.run(title, [&]() {
      for (size_t i = 0; i < data.size(); i++) {
        data[i] = romu64_random_edit_r(&romu_rng);
      }
    });

    title = std::string{"romu edit2 ("} + bench_num + ")";
    benchmark.run(title, [&]() {
      for (size_t i = 0; i < data.size(); i++) {
        data[i] = romu64_random_edit2_r(&romu_rng);
      }
    });

    title = std::string{"pcg32 ("} + bench_num + ")";
    benchmark.run(title, [&]() {
      for (size_t i = 0; i < data.size(); i++) {
        data[i] = pcg32_random_r(&rng);
      }
    });
    
    title = std::string{"rand ("} + bench_num + ")";
    benchmark.run(title, [&]() {
      for (size_t i = 0; i < data.size(); i++) {
        data[i] = rand();
      }
    });

    title = std::string{"nanobench/romu ("} + bench_num + ")";
    benchmark.run(title, [&]() {
      for (size_t i = 0; i < data.size(); i++) {
        data[i] = std::uniform_int_distribution<uint64_t>{}(ank_rng);
      }
    });

    title = std::string{"mersene twister ("} + bench_num + ")";
    benchmark.run(title, [&]() {
      for (size_t i = 0; i < data.size(); i++) {
        data[i] = std::uniform_int_distribution<uint64_t>{}(mt_rng);
      }
    });
    
    bench_size += 1024;
  }
}
