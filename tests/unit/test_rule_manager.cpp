// tests/unit/test_rule_manager.cpp — UT-15 ~ UT-22

#include <gtest/gtest.h>
#include "shield/RuleManager.hpp"

using shield::RuleManager;
using shield::RuleLoadError;

static const std::string VALID_JSON = R"json({
  "version": "1.0",
  "rules": [
    {
      "id": "rule_password",
      "description": "Password masking",
      "trigger_keywords": ["password"],
      "pattern": "(?i)password=(\\S+)",
      "mask_group": 1,
      "replacement": "***"
    }
  ]
})json";

static const std::string MULTI_RULE_JSON = R"json({
  "version": "1.0",
  "rules": [
    {
      "id": "rule_password",
      "trigger_keywords": ["password"],
      "pattern": "(?i)password=(\\S+)",
      "mask_group": 1,
      "replacement": "***"
    },
    {
      "id": "rule_token",
      "trigger_keywords": ["token"],
      "pattern": "token=(\\S+)",
      "mask_group": 1,
      "replacement": "***"
    }
  ]
})json";

static const std::string INVALID_REGEX_JSON = R"json({
  "version": "1.0",
  "rules": [
    {
      "id": "bad_rule",
      "trigger_keywords": ["x"],
      "pattern": "(?P<bad",
      "mask_group": 0,
      "replacement": "***"
    },
    {
      "id": "good_rule",
      "trigger_keywords": ["password"],
      "pattern": "password=(\\S+)",
      "mask_group": 1,
      "replacement": "***"
    }
  ]
})json";

// UT-15: 정상 JSON 로드 → nullptr 아님
TEST(RuleManagerTest, LoadValidJson) {
    auto mgr = RuleManager::from_json(VALID_JSON);
    ASSERT_NE(mgr, nullptr);
    auto rs = mgr->get_current_ruleset();
    EXPECT_NE(rs, nullptr);
    EXPECT_GE(rs->rule_count(), 1u);
}

// UT-16: rules 배열 누락 JSON → 예외
TEST(RuleManagerTest, MissingRulesField) {
    EXPECT_THROW(
        RuleManager::from_json(R"json({"version":"1.0"})json"),
        RuleLoadError
    );
}

// UT-17: 잘못된 정규식 → 해당 룰 스킵, 유효한 룰은 등록됨
TEST(RuleManagerTest, InvalidRegexSkipped) {
    auto mgr = RuleManager::from_json(INVALID_REGEX_JSON);
    ASSERT_NE(mgr, nullptr);
    auto rs = mgr->get_current_ruleset();
    ASSERT_NE(rs, nullptr);
    // good_rule 1개만 컴파일됨
    EXPECT_EQ(rs->rule_count(), 1u);
}

// UT-18: update_rules 후 새 룰셋 반영
TEST(RuleManagerTest, UpdateRulesSwapsRuleset) {
    auto mgr = RuleManager::from_json(VALID_JSON);
    auto rs1 = mgr->get_current_ruleset();

    mgr->update_rules(MULTI_RULE_JSON);
    auto rs2 = mgr->get_current_ruleset();

    EXPECT_NE(rs1, rs2);
    EXPECT_EQ(rs2->rule_count(), 2u);
}

// UT-22: 기본 룰셋 5종 파싱 성공
TEST(RuleManagerTest, DefaultRulesFileParsed) {
    static const std::string DEFAULT_LIKE_JSON = R"json({
      "version": "1.0",
      "rules": [
        {"id":"rule_password","trigger_keywords":["password","passwd","pwd"],
         "pattern":"(?i)(?:password|passwd|pwd)=(\\S+)","mask_group":1,"replacement":"***"},
        {"id":"rule_bearer","trigger_keywords":["Bearer","Authorization"],
         "pattern":"Bearer\\s+([A-Za-z0-9\\-._~+/]+=*)","mask_group":1,"replacement":"***REDACTED***"},
        {"id":"rule_email","trigger_keywords":["@"],
         "pattern":"[a-zA-Z0-9._%+\\-]+@[a-zA-Z0-9.\\-]+\\.[a-zA-Z]{2,}","mask_group":0,"replacement":"***@***.***"},
        {"id":"rule_phone_kr","trigger_keywords":["010","011","016","017","018","019"],
         "pattern":"01[016789]-?(\\d{3,4})-?(\\d{4})","mask_group":0,"replacement":"***-****-****"},
        {"id":"rule_credit_card","trigger_keywords":["card","credit"],
         "pattern":"\\b(\\d{4})[- ]?(\\d{4})[- ]?(\\d{4})[- ]?(\\d{4})\\b","mask_group":0,"replacement":"****-****-****-****"}
      ]
    })json";
    auto mgr = RuleManager::from_json(DEFAULT_LIKE_JSON);
    ASSERT_NE(mgr, nullptr);
    auto rs = mgr->get_current_ruleset();
    ASSERT_NE(rs, nullptr);
    EXPECT_EQ(rs->rule_count(), 5u);
}
