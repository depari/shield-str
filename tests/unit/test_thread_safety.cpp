// tests/unit/test_thread_safety.cpp — UT-19 ~ UT-21
#include <gtest/gtest.h>
#include "shield/MaskingEngine.hpp"
#include "shield/RuleManager.hpp"
#include <atomic>
#include <string>
#include <thread>
#include <vector>
#include <chrono>

using shield::MaskingEngine;
using shield::RuleManager;

static const std::string RULE_V1 = R"json({
  "version": "1.0",
  "rules": [
    {
      "id": "rule_password",
      "trigger_keywords": ["password"],
      "pattern": "(?i)(?:password|passwd|pwd)[\"']?\\s*[:=]\\s*[\"']?([^\\s\"']+)",
      "mask_group": 1,
      "replacement": "***"
    }
  ]
})json";

static const std::string RULE_V2 = R"json({
  "version": "2.0",
  "rules": [
    {
      "id": "rule_password",
      "trigger_keywords": ["password"],
      "pattern": "(?i)(?:password|passwd|pwd)[\"']?\\s*[:=]\\s*[\"']?([^\\s\"']+)",
      "mask_group": 1,
      "replacement": "[REDACTED]"
    }
  ]
})json";

static std::string to_str(const MaskingEngine::Result& r) {
    if (std::holds_alternative<std::string_view>(r)) {
        return std::string(std::get<std::string_view>(r));
    }
    return std::get<std::string>(r);
}

// UT-19: 여러 스레드에서 동시에 마스킹과 룰 업데이트가 발생해도 크래시가 없어야 함
TEST(ThreadSafetyTest, ConcurrentProcessAndUpdate) {
    auto mgr    = RuleManager::from_json(RULE_V1);
    auto engine = std::make_shared<MaskingEngine>(mgr);
    
    std::atomic<bool> running{true};
    std::atomic<int> errors{0};

    // Reader threads
    std::vector<std::thread> readers;
    for (int i = 0; i < 4; ++i) {
        readers.emplace_back([&]() {
            while (running) {
                auto res = engine->process("password=secret123");
                std::string s = to_str(res);
                if (s.find("secret123") != std::string::npos) {
                    errors++;
                }
            }
        });
    }

    // Writer thread (updates rules)
    std::thread writer([&]() {
        int count = 0;
        while (running) {
            mgr->update_rules(RULE_V1);
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            mgr->update_rules(RULE_V2);
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            if (++count > 20) break;
        }
        running = false;
    });

    writer.join();
    for (auto& t : readers) t.join();

    EXPECT_EQ(errors.load(), 0);
}

// UT-20: 업데이트 직후 새 룰셋으로 마스킹이 즉시 적용되어야 함
TEST(ThreadSafetyTest, UpdateImmediatelyEffective) {
    auto mgr    = RuleManager::from_json(RULE_V1);
    auto engine = std::make_shared<MaskingEngine>(mgr);

    // V1: replacement = "***"
    {
        auto result = engine->process("password=secret");
        std::string s = to_str(result);
        EXPECT_TRUE(s.find("***") != std::string::npos);
    }

    // Switch to V2: replacement = "[REDACTED]"
    mgr->update_rules(RULE_V2);
    {
        auto result = engine->process("password=secret");
        std::string s = to_str(result);
        EXPECT_TRUE(s.find("[REDACTED]") != std::string::npos);
    }
}
