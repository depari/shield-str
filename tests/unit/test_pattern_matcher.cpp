// tests/unit/test_re2_matcher.cpp — UT-06 ~ UT-10, UT-23, UT-24

#include <gtest/gtest.h>
#include "shield/PatternMatcher.hpp"
#ifdef SHIELD_USE_STD_ONLY
#include <regex>
#else
#include <re2/re2.h>
#endif

using shield::PatternMatcher;

static PatternMatcher::Rule make_rule(
    const std::string& id,
    const std::string& pattern,
    int                mask_group,
    const std::string& replacement = "***")
{
    PatternMatcher::Rule r;
    r.id          = id;
#ifdef SHIELD_USE_STD_ONLY
    auto flags = std::regex_constants::ECMAScript;
    std::string pat = pattern;
    if (pat.find("(?i)") == 0) {
        flags |= std::regex_constants::icase;
        pat = pat.substr(4);
    }
    r.pattern     = std::make_unique<std::regex>(pat, flags);
#else
    r.pattern     = std::make_unique<re2::RE2>(pattern);
#endif
    r.mask_group  = mask_group;
    r.replacement = replacement;
    return r;
}

// UT-06: Password 마스킹
TEST(PatternMatcherTest, PasswordMasking) {
    PatternMatcher m;
    m.add_rule(make_rule("pw", R"((?i)(?:password|passwd|pwd)["']?\s*[:=]\s*["']?(\S+))", 1));
    auto result = m.apply("password=secret123 user=alice");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->find("secret123"), std::string::npos);
    EXPECT_TRUE(result->find("***") != std::string::npos);
    EXPECT_TRUE(result->find("user=alice") != std::string::npos);
}

// UT-07: Bearer Token 마스킹
TEST(PatternMatcherTest, BearerTokenMasking) {
    PatternMatcher m;
    m.add_rule(make_rule("bearer",
        R"(Bearer\s+([A-Za-z0-9\-._~+/]+=*))", 1, "***REDACTED***"));
    auto result = m.apply("Authorization: Bearer eyJhbGciOiJSUzI1NiJ9.abc");
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(result->find("***REDACTED***") != std::string::npos);
    EXPECT_TRUE(result->find("eyJ") == std::string::npos);
}

// UT-08: 이메일 마스킹
TEST(PatternMatcherTest, EmailMasking) {
    PatternMatcher m;
    m.add_rule(make_rule("email",
        R"([a-zA-Z0-9._%+\-]+@[a-zA-Z0-9.\-]+\.[a-zA-Z]{2,})", 0, "***@***.***"));
    auto result = m.apply("email: user@example.com logged in");
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(result->find("***@***.***") != std::string::npos);
    EXPECT_TRUE(result->find("user@example.com") == std::string::npos);
}

// UT-09: 마스킹 대상 없음 → nullopt
TEST(PatternMatcherTest, NoMatchReturnsNullopt) {
    PatternMatcher m;
    m.add_rule(make_rule("pw", R"(password=(\S+))", 1));
    auto result = m.apply("user=alice id=1234");
    EXPECT_FALSE(result.has_value());
}

// UT-10: 복수 패턴 동시 적용
TEST(PatternMatcherTest, MultipleRulesApplied) {
    PatternMatcher m;
    m.add_rule(make_rule("pw",
        R"((?i)(?:password|passwd|pwd)["\']?\s*[:=]\s*["\']?([^\s"\']+))", 1));
    m.add_rule(make_rule("email",
        R"([a-zA-Z0-9._%+\-]+@[a-zA-Z0-9.\-]+\.[a-zA-Z]{2,})", 0, "***@***.***"));
    auto result = m.apply("password=secret user@example.com connected");
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(result->find("***") != std::string::npos);
    EXPECT_TRUE(result->find("***@***.***") != std::string::npos);
}

// UT-23: 한국 전화번호 마스킹
TEST(PatternMatcherTest, KoreanPhoneMasking) {
    PatternMatcher m;
    m.add_rule(make_rule("phone_kr",
        R"(01[016789]-?(\d{3,4})-?(\d{4}))", 0, "***-****-****"));
    auto result = m.apply("call 010-1234-5678 now");
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(result->find("***-****-****") != std::string::npos);
    EXPECT_TRUE(result->find("1234") == std::string::npos);
}

// UT-24: 신용카드 번호 마스킹
TEST(PatternMatcherTest, CreditCardMasking) {
    PatternMatcher m;
    m.add_rule(make_rule("cc",
        R"(\b(\d{4})[- ]?(\d{4})[- ]?(\d{4})[- ]?(\d{4})\b)", 0, "****-****-****-****"));
    auto result = m.apply("card: 4111-1111-1111-1111 expires 12/26");
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(result->find("****-****-****-****") != std::string::npos);
    EXPECT_TRUE(result->find("4111") == std::string::npos);
}

// 추가: 빈 룰셋 → nullopt
TEST(PatternMatcherTest, EmptyRuleSet) {
    PatternMatcher m;
    EXPECT_FALSE(m.apply("anything").has_value());
}
