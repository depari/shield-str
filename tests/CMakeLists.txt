cmake_minimum_required(VERSION 3.16)

# ── Unit Tests ────────────────────────────────────────────────────────────────
add_executable(shield_unit_tests
    unit/test_aho_corasick.cpp
    unit/test_re2_matcher.cpp
    unit/test_rule_manager.cpp
    unit/test_masking_engine.cpp
    unit/test_thread_safety.cpp
)

target_link_libraries(shield_unit_tests
    PRIVATE
        shield::shield-str
        GTest::gtest_main
)

target_compile_options(shield_unit_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra>
)

include(GoogleTest)
gtest_discover_tests(shield_unit_tests)
