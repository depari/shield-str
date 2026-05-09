// BM-01: 10KB 문자열, 마스킹 대상 없음 — 목표: p99 < 100ns

#include <benchmark/benchmark.h>
#include "shield/MaskingEngine.hpp"
#include "shield/RuleManager.hpp"
#include <string>

static const char* RULE_JSON = R"json({
  "version": "1.0",
  "rules": [
    {
      "id": "rule_password",
      "trigger_keywords": ["password", "passwd", "pwd"],
      "pattern": "(?i)(?:password|passwd|pwd)[\"\']?\\s*[:=]\\s*[\"\']?([^\\s\"\']+)",
      "mask_group": 1,
      "replacement": "***"
    }
  ]
})json";

// 10KB log line with no sensitive data
static std::string make_clean_log() {
    std::string s;
    s.reserve(10240);
    while (s.size() < 10240) {
        s += "2026-05-09T10:00:00Z INFO user=alice action=login status=ok session=abcdef ";
    }
    return s.substr(0, 10240);
}

static void BM_NoMask(benchmark::State& state) {
    auto mgr    = shield::RuleManager::from_json(RULE_JSON);
    auto engine = std::make_shared<shield::MaskingEngine>(mgr);
    auto log    = make_clean_log();

    for (auto _ : state) {
        auto result = engine->process(log);
        benchmark::DoNotOptimize(result);
    }
    state.SetBytesProcessed(
        static_cast<int64_t>(state.iterations()) * static_cast<int64_t>(log.size()));
}

BENCHMARK(BM_NoMask)->Iterations(100000)->Unit(benchmark::kNanosecond);
