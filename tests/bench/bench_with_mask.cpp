// BM-02: 10KB 문자열, 마스킹 대상 1건 — 목표: p99 < 1ms

#include <benchmark/benchmark.h>
#include "shield/MaskingEngine.hpp"
#include "shield/RuleManager.hpp"
#include <string>

static const char* RULE_JSON = R"json({
  "version": "1.0",
  "rules": [
    {
      "id": "rule_password",
      "trigger_keywords": ["password"],
      "pattern": "(?i)password=(\\S+)",
      "mask_group": 1,
      "replacement": "***"
    }
  ]
})json";

static std::string make_log_with_password() {
    std::string s;
    s.reserve(10240);
    while (s.size() < 5000) {
        s += "2026-05-09T10:00:00Z INFO user=alice action=login status=ok ";
    }
    s += "password=SuperSecret123 ";
    while (s.size() < 10240) {
        s += "session=xyz request_id=abc ";
    }
    return s.substr(0, 10240);
}

static void BM_WithMask(benchmark::State& state) {
    auto mgr    = shield::RuleManager::from_json(RULE_JSON);
    auto engine = std::make_shared<shield::MaskingEngine>(mgr);
    auto log    = make_log_with_password();

    for (auto _ : state) {
        auto result = engine->process(log);
        benchmark::DoNotOptimize(result);
    }
    state.SetBytesProcessed(
        static_cast<int64_t>(state.iterations()) * static_cast<int64_t>(log.size()));
}

BENCHMARK(BM_WithMask)->Iterations(10000)->Unit(benchmark::kMicrosecond);
