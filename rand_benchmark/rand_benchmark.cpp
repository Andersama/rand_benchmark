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

int main() {
  std::chrono::high_resolution_clock clk;

  pcg32_random_t rng;
  rng.inc = std::chrono::duration_cast<std::chrono::nanoseconds>(
                clk.now().time_since_epoch())
                .count();

  std::random_device dev;
  std::mt19937_64 mt_rng(dev());
  ankerl::nanobench::Rng ank_rng(dev());

  ankerl::nanobench::Bench benchmark;
  // benchmark.epochs(1024);
  // benchmark.minEpochIterations(128);
  // benchmark.minEpochTime(std::chrono::nanoseconds{4000});
  benchmark.warmup(1);
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

  while (bench_size < target_size) {
    benchmark.batch(bench_size);
    while (data.size() < bench_size) {
      data.emplace_back();
    }
    for (size_t i = 0; i < data.size(); i++) {
      data[i] = pcg32_random_r(&rng);
    }

    std::string bench_num = std::to_string(bench_size);
    std::string title = std::string{"pcg32 ("} + bench_num + ")";
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

    title = std::string{"nanobench ("} + bench_num + ")";
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
