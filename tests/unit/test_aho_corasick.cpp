// tests/unit/test_aho_corasick.cpp — UT-01 ~ UT-05

#include <gtest/gtest.h>
#include "shield/AhoCorasickScanner.hpp"

using shield::AhoCorasickScanner;

// UT-01: 단일 키워드 탐지
TEST(AhoCorasickScannerTest, SingleKeywordFound) {
    AhoCorasickScanner s;
    s.build({"password"});
    EXPECT_TRUE(s.scan("user=admin password=1234"));
}

// UT-02: 다중 키워드 동시 탐지
TEST(AhoCorasickScannerTest, MultipleKeywordsFound) {
    AhoCorasickScanner s;
    s.build({"password", "token"});
    EXPECT_TRUE(s.scan("token=abc def"));
    EXPECT_TRUE(s.scan("password=abc"));
    EXPECT_FALSE(s.scan("user=alice id=1"));
}

// UT-03: 키워드 없는 문자열 → false
TEST(AhoCorasickScannerTest, NoKeywordNotFound) {
    AhoCorasickScanner s;
    s.build({"password"});
    EXPECT_FALSE(s.scan("user=admin id=1234"));
}

// UT-04: 빈 키워드 셋 → false
TEST(AhoCorasickScannerTest, EmptyKeywordSet) {
    AhoCorasickScanner s;
    s.build({});
    EXPECT_FALSE(s.scan("anything here"));
    EXPECT_TRUE(s.empty());
}

// UT-05: 대소문자 구분 (기본: 구분함)
TEST(AhoCorasickScannerTest, CaseSensitive) {
    AhoCorasickScanner s;
    s.build({"password"});
    // 소문자 키워드는 찾음
    EXPECT_TRUE(s.scan("password=1234"));
    // 대문자는 찾지 못함 (case-sensitive)
    EXPECT_FALSE(s.scan("PASSWORD=1234"));
}

// 추가: 키워드가 문자열 경계에 있는 경우
TEST(AhoCorasickScannerTest, KeywordAtBoundary) {
    AhoCorasickScanner s;
    s.build({"token"});
    EXPECT_TRUE(s.scan("token"));
    EXPECT_TRUE(s.scan("xtoken"));
    EXPECT_TRUE(s.scan("tokenx"));
}

// 추가: 빈 입력 문자열
TEST(AhoCorasickScannerTest, EmptyInput) {
    AhoCorasickScanner s;
    s.build({"password"});
    EXPECT_FALSE(s.scan(""));
}
