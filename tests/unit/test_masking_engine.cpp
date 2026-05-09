// tests/unit/test_masking_engine.cpp — UT-11 ~ UT-14

#include <gtest/gtest.h>
#include "shield/MaskingEngine.hpp"
#include "shield/RuleManager.hpp"
#include <variant>
#include <string>
#include <string_view>

using shield::MaskingEngine;
using shield::RuleManager;

static const std::string PW_RULE_JSON = R"json({
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

// Helper: extract string result from variant
static std::string to_str(const MaskingEngine::Result& r) {
    if (std::holds_alternative<std::string_view>(r)) {
        return std::string(std::get<std::string_view>(r));
    }
    return std::get<std::string>(r);
}

static bool is_zero_copy(const MaskingEngine::Result& r) {
    return std::holds_alternative<std::string_view>(r);
}

// UT-11: 마스킹 대상 없음 → string_view 반환 (Zero-Copy)
TEST(MaskingEngineTest, NoMaskReturnsStringView) {
    auto mgr    = RuleManager::from_json(PW_RULE_JSON);
    auto engine = std::make_shared<MaskingEngine>(mgr);

    auto result = engine->process("user=alice id=1234");
    EXPECT_TRUE(is_zero_copy(result));
    EXPECT_EQ(to_str(result), "user=alice id=1234");
}

// UT-12: 마스킹 대상 있음 → string 반환 (새 버퍼)
TEST(MaskingEngineTest, MaskReturnsString) {
    auto mgr    = RuleManager::from_json(PW_RULE_JSON);
    auto engine = std::make_shared<MaskingEngine>(mgr);

    auto result = engine->process("password=secret123 user=alice");
    EXPECT_FALSE(is_zero_copy(result));
    EXPECT_TRUE(to_str(result).find("***") != std::string::npos);
    EXPECT_TRUE(to_str(result).find("secret123") == std::string::npos);
}

// UT-25: Colon 기반의 비밀번호 마스킹 지원 (고도화 반영)
TEST(MaskingEngineTest, PasswordWithColonMasking) {
    auto mgr    = RuleManager::from_json(PW_RULE_JSON);
    auto engine = std::make_shared<MaskingEngine>(mgr);

    auto result = engine->process("password:eondlkajsdfasd status=ok");
    EXPECT_FALSE(is_zero_copy(result));
    EXPECT_TRUE(to_str(result).find("***") != std::string::npos);
    EXPECT_TRUE(to_str(result).find("eondlkajsdfasd") == std::string::npos);
}

static const std::string AUTH_RULE_JSON = R"json({
  "version": "1.0",
  "rules": [
    {
      "id": "rule_auth_token",
      "trigger_keywords": ["authtoken", "authcode", "auth_token", "auth_code"],
      "pattern": "(?i)(?:authtoken|authcode|auth_token|auth_code)[\"']?\\s*[:=]\\s*[\"']?([^\\s\"']+)",
      "mask_group": 1,
      "replacement": "***AUTH***"
    }
  ]
})json";

// UT-26: AuthToken / AuthCode 기반의 인증 키 마스킹 지원
TEST(MaskingEngineTest, AuthTokenMasking) {
    auto mgr    = RuleManager::from_json(AUTH_RULE_JSON);
    auto engine = std::make_shared<MaskingEngine>(mgr);

    auto result1 = engine->process("authtoken=ABC123456XYZ");
    EXPECT_FALSE(is_zero_copy(result1));
    EXPECT_TRUE(to_str(result1).find("***AUTH***") != std::string::npos);
    EXPECT_TRUE(to_str(result1).find("ABC123456XYZ") == std::string::npos);

    auto result2 = engine->process("authcode : secret_token_99");
    EXPECT_FALSE(is_zero_copy(result2));
    EXPECT_TRUE(to_str(result2).find("***AUTH***") != std::string::npos);
    EXPECT_TRUE(to_str(result2).find("secret_token_99") == std::string::npos);
}

// UT-13: 빈 문자열 입력 → 빈 string_view 반환
TEST(MaskingEngineTest, EmptyInputReturnsEmpty) {
    auto mgr    = RuleManager::from_json(PW_RULE_JSON);
    auto engine = std::make_shared<MaskingEngine>(mgr);

    auto result = engine->process("");
    EXPECT_TRUE(is_zero_copy(result));
    EXPECT_EQ(to_str(result), "");
}

// UT-14: JSON 포맷 로그 마스킹
TEST(MaskingEngineTest, JsonFormatLogMasking) {
    static const std::string JSON_RULE = R"json({
      "version": "1.0",
      "rules": [
        {
          "id": "rule_password",
          "trigger_keywords": ["password"],
          "pattern": "\"password\"\\s*:\\s*\"([^\"]+)\"",
          "mask_group": 1,
          "replacement": "***"
        }
      ]
    })json";
    auto mgr    = RuleManager::from_json(JSON_RULE);
    auto engine = std::make_shared<MaskingEngine>(mgr);

    auto result = engine->process(R"({"user":"alice","password":"s3cr3t"})");
    EXPECT_FALSE(is_zero_copy(result));
    auto s = to_str(result);
    EXPECT_TRUE(s.find("***") != std::string::npos);
    EXPECT_TRUE(s.find("s3cr3t") == std::string::npos);
    EXPECT_TRUE(s.find("alice") != std::string::npos);
}

// 추가: null rule_manager → 예외
TEST(MaskingEngineTest, NullRuleManagerThrows) {
    EXPECT_THROW(
        MaskingEngine(nullptr),
        std::invalid_argument
    );
}
