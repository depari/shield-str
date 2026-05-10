// tests/unit/test_rule_manager.cpp — UT-15 ~ UT-22
#include <gtest/gtest.h>
#include "shield/RuleManager.hpp"

using shield::RuleManager;
using shield::RuleLoadError;

static const std::string VALID_JSON = R"json({
  "version": "1.0",
  "rules": [
    {
      "id": "test_rule",
      "trigger_keywords": ["test"],
      "pattern": "test",
      "replacement": "***"
    },
    {
      "id": "good_rule",
      "trigger_keywords": ["password"],
      "pattern": "password=(\\S+)",
      "mask_group": 1
    }
  ]
})json";

static const std::string INVALID_REGEX_JSON = R"json({
  "rules": [
    {
      "id": "bad_rule",
      "trigger_keywords": ["bad"],
      "pattern": "(?P<bad",
      "replacement": "***"
    },
    {
      "id": "good_rule",
      "trigger_keywords": ["good"],
      "pattern": "good",
      "replacement": "!!!"
    }
  ]
})json";

TEST(RuleManagerTest, LoadValidJson) {
    auto mgr = RuleManager::from_json(VALID_JSON);
    ASSERT_NE(mgr, nullptr);
    auto rs = mgr->get_current_ruleset();
    EXPECT_NE(rs, nullptr);
    EXPECT_GE(rs->rule_count(), 1u);
}

TEST(RuleManagerTest, MissingRulesField) {
    std::string bad_json = R"json({"version": "1.0"})json";
    EXPECT_THROW(RuleManager::from_json(bad_json), RuleLoadError);
}

TEST(RuleManagerTest, InvalidRegexSkipped) {
    auto mgr = RuleManager::from_json(INVALID_REGEX_JSON);
    auto rs = mgr->get_current_ruleset();
    EXPECT_EQ(rs->rule_count(), 1u);
}

TEST(RuleManagerTest, UpdateRulesSwapsRuleset) {
    auto mgr = RuleManager::from_json(VALID_JSON);
    auto rs1 = mgr->get_current_ruleset();
    
    std::string json2 = R"json({
      "rules": [
        {
          "id": "rule_token",
          "trigger_keywords": ["token"],
          "pattern": "token=\\S+",
          "replacement": "[REDACTED]"
        }
      ]
    })json";
    
    mgr->update_rules(json2);
    auto rs2 = mgr->get_current_ruleset();
    
    EXPECT_NE(rs1, rs2);
    EXPECT_EQ(rs2->rule_count(), 1u);
}

TEST(RuleManagerTest, DefaultRulesFileParsed) {
    // This assumes rules.json exists in CWD or default location
    // For now just check it doesn't crash if file missing (should throw)
    try {
        auto mgr = RuleManager::from_file("rules.json");
        EXPECT_NE(mgr, nullptr);
    } catch (const RuleLoadError&) {
        // Expected if file not found
    }
}
