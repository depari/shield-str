// shield/PCRE2Wrapper.hpp
#pragma once
#include <string>
#include <string_view>
#include <vector>

#ifdef SHIELD_REGEX_PCRE2
#define PCRE2_CODE_UNIT_WIDTH 8
#include <pcre2.h>

namespace shield {

class PCRE2Regex {
public:
    explicit PCRE2Regex(const std::string& pattern);
    ~PCRE2Regex();

    PCRE2Regex(const PCRE2Regex&) = delete;
    PCRE2Regex& operator=(const PCRE2Regex&) = delete;
    PCRE2Regex(PCRE2Regex&&) noexcept;
    PCRE2Regex& operator=(PCRE2Regex&&) noexcept;

    bool ok() const { return code_ != nullptr; }
    
    // Unified interface for PatternMatcher
    int match_groups(std::string_view subject, std::vector<std::string_view>& groups) const;

private:
    pcre2_code* code_ = nullptr;
};

} // namespace shield
#endif
