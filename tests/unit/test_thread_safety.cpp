// tests/unit/test_thread_safety.cpp — UT-19 ~ UT-21

#include <gtest/gtest.h>
#include "shield/MaskingEngine.hpp"
#include "shield/RuleManager.hpp"
#include <atomic>
#include <string>
#include <thread>
#include <vector>

using shield::MaskingEngine;
using shield::RuleManager;

static const std::string RULE_V1 = R"json({
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

static const std::string RULE_V2 = R"json({
  "version": "2.0",
  "rules": [
    {
      "id": "rule_password",
      "trigger_keywords": ["password"],
      "pattern": "(?i)password=(\\S+)",
      "mask_group": 1,
      "replacement": "[REDACTED]"
    },
    {
      "id": "rule_token",
      "trigger_keywords": ["token"],
      "pattern": "token=(\\S+)",
      "mask_group": 1,
      "replacement": "[REDACTED]"
    }
  ]
})json";

// UT-19: 64스레드 동시 process() 호출 중 update_rules() — Data Race 없음
TEST(ThreadSafetyTest, ConcurrentProcessAndUpdate) {
    auto mgr    = RuleManager::from_json(RULE_V1);
    auto engine = std::make_shared<MaskingEngine>(mgr);

    const int          num_readers = 64;
    const int          iterations  = 500;
    std::atomic<int>   errors{0};

    std::vector<std::thread> readers;
    readers.reserve(num_readers);
    for (int i = 0; i < num_readers; ++i) {
        readers.emplace_back([&]() {
            for (int n = 0; n < iterations; ++n) {
                try {
                    auto r = engine->process("user=alice password=secret token=abc");
                    (void)r;
                } catch (...) {
                    ++errors;
                }
            }
        });
    }

    std::thread writer([&]() {
        for (int n = 0; n < 10; ++n) {
            mgr->update_rules((n % 2 == 0) ? RULE_V2 : RULE_V1);
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    });

    for (auto& t : readers) t.join();
    writer.join();

    EXPECT_EQ(errors.load(), 0);
}

// UT-20: 업데이트 직후 새 룰셋으로 마스킹
TEST(ThreadSafetyTest, UpdateImmediatelyEffective) {
    auto mgr    = RuleManager::from_json(RULE_V1);
    auto engine = std::make_shared<MaskingEngine>(mgr);

    // V1: replacement = "***"
    {
        auto result = engine->process("password=secret");
        std::string s;
        if (std::holds_alternative<std::string>(result))
            s = std::get<std::string>(result);
        EXPECT_TRUE(s.find("***") != std::string::npos);
    }

    // Switch to V2: replacement = "[REDACTED]"
    mgr->update_rules(RULE_V2);
    {
        auto result = engine->process("password=secret");
        std::string s;
        if (std::holds_alternative<std::string>(result))
            s = std::get<std::string>(result);
        EXPECT_TRUE(s.find("[REDACTED]") != std::string::npos);
    }
}

// UT-21: 이전 룰셋 참조 중인 스레드가 안전하게 완료
TEST(ThreadSafetyTest, OldRulesetSafelyReleased) {
    auto mgr = RuleManager::from_json(RULE_V1);

    auto old_rs = mgr->get_current_ruleset();
    ASSERT_NE(old_rs, nullptr);
    auto old_ptr = old_rs.get();

    mgr->update_rules(RULE_V2);
    auto new_rs = mgr->get_current_ruleset();
    ASSERT_NE(new_rs, nullptr);

    EXPECT_NE(old_ptr, new_rs.get());
    // old_rs still valid — shared ownership maintained
    EXPECT_NE(old_rs.get(), nullptr);
}
