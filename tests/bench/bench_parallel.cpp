// BM-04: 64스레드 병렬 Throughput 측정

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

static std::shared_ptr<shield::MaskingEngine> g_engine;

static void BM_Parallel(benchmark::State& state) {
    static std::once_flag init;
    std::call_once(init, []() {
        auto mgr = shield::RuleManager::from_json(RULE_JSON);
        g_engine = std::make_shared<shield::MaskingEngine>(mgr);
    });

    const std::string log = "user=alice password=secret action=login status=ok";

    for (auto _ : state) {
        auto result = g_engine->process(log);
        benchmark::DoNotOptimize(result);
    }
    state.SetItemsProcessed(state.iterations());
}

BENCHMARK(BM_Parallel)->Threads(64)->Unit(benchmark::kNanosecond);

#include <mutex>
