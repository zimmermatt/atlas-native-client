#include <rapidjson/document.h>
#include <benchmark/benchmark.h>
#include "../../meter/subscription.h"

// rapidjson::Document::ParseInsitu(buffer) is more efficient than the
// Parse version (by around 12-13% on a payload with around 30k subscriptions)
//
// Run on (8 X 2793.31 MHz CPU s)
// 2018-07-13 22:00:56
// --------------------------------------------------
// Benchmark           Time           CPU Iterations
// --------------------------------------------------
// BM_insitu    37849961 ns   37850002 ns         19
// BM_copy      43675883 ns   43676134 ns         16

using atlas::meter::Subscription;
using atlas::meter::Subscriptions;

char* read_file(const char* fname) {
  auto fp = fopen(fname, "r");
  assert(fp != nullptr);
  fseek(fp, 0, SEEK_END);
  auto filesize = static_cast<size_t>(ftell(fp));
  fseek(fp, 0, SEEK_SET);
  auto buffer = static_cast<char*>(malloc(filesize + 1));
  auto readLength = fread(buffer, 1, filesize, fp);
  buffer[readLength] = '\0';
  fclose(fp);
  return buffer;
}

static Subscriptions load_subs(char* buffer) {
  Subscriptions result;
  rapidjson::Document d;
  d.Parse(buffer);
  free(buffer);

  if (d.IsObject()) {
    auto expressions = d["expressions"].GetArray();
    for (auto& e : expressions) {
      if (e.IsObject()) {
        auto expr = e.GetObject();
        auto freq = expr["frequency"].GetInt64();
        result.emplace_back(Subscription{expr["id"].GetString(), freq,
                                         expr["expression"].GetString()});
      }
    }
  }

  return result;
}

static Subscriptions load_subs_insitu(char* buffer) {
  Subscriptions result;

  rapidjson::Document d;
  d.ParseInsitu(buffer);

  if (d.IsObject()) {
    auto expressions = d["expressions"].GetArray();
    for (auto& e : expressions) {
      if (e.IsObject()) {
        auto expr = e.GetObject();
        auto freq = expr["frequency"].GetInt64();
        result.emplace_back(Subscription{expr["id"].GetString(), freq,
                                         expr["expression"].GetString()});
      }
    }
  }

  free(buffer);
  return result;
}

static void BM_copy(benchmark::State& state) {
  auto contents = read_file("subs.json");
  for (auto _ : state) {
    auto buffer = strdup(contents);
    benchmark::DoNotOptimize(load_subs(buffer));
  }
  free(contents);
}

static void BM_insitu(benchmark::State& state) {
  auto contents = read_file("subs.json");
  for (auto _ : state) {
    auto buffer = strdup(contents);
    benchmark::DoNotOptimize(load_subs_insitu(buffer));
  }
  free(contents);
}

BENCHMARK(BM_insitu);
BENCHMARK(BM_copy);
BENCHMARK_MAIN();
