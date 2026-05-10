// shield/PCRE2Wrapper.cpp
#include "PCRE2Wrapper.hpp"

#ifdef SHIELD_REGEX_PCRE2

namespace shield {

PCRE2Regex::PCRE2Regex(const std::string& pattern) {
    int errornumber;
    PCRE2_SIZE erroroffset;

    uint32_t options = PCRE2_UTF | PCRE2_UCP;
    std::string final_pattern = pattern;
    if (pattern.find("(?i)") == 0) {
        options |= PCRE2_CASELESS;
        final_pattern = pattern.substr(4);
    }

    code_ = pcre2_compile(
        reinterpret_cast<PCRE2_SPTR>(final_pattern.c_str()),
        PCRE2_ZERO_TERMINATED,
        options,
        &errornumber,
        &erroroffset,
        nullptr
    );

    if (code_) {
        pcre2_jit_compile(code_, PCRE2_JIT_COMPLETE);
    }
}

PCRE2Regex::~PCRE2Regex() {
    if (code_) {
        pcre2_code_free(code_);
    }
}

PCRE2Regex::PCRE2Regex(PCRE2Regex&& other) noexcept : code_(other.code_) {
    other.code_ = nullptr;
}

PCRE2Regex& PCRE2Regex::operator=(PCRE2Regex&& other) noexcept {
    if (this != &other) {
        if (code_) pcre2_code_free(code_);
        code_ = other.code_;
        other.code_ = nullptr;
    }
    return *this;
}

int PCRE2Regex::match_groups(std::string_view subject, std::vector<std::string_view>& groups) const {
    if (!code_) return -1;

    pcre2_match_data* match_data = pcre2_match_data_create_from_pattern(code_, nullptr);
    int rc = pcre2_match(
        code_,
        reinterpret_cast<PCRE2_SPTR>(subject.data()),
        subject.size(),
        0,
        0,
        match_data,
        nullptr
    );

    if (rc < 0) {
        pcre2_match_data_free(match_data);
        return rc;
    }

    PCRE2_SIZE* ovector = pcre2_get_ovector_pointer(match_data);
    groups.clear();
    for (int i = 0; i < rc; i++) {
        size_t start = ovector[2 * i];
        size_t end = ovector[2 * i + 1];
        if (start == PCRE2_UNSET || end == PCRE2_UNSET) {
            groups.push_back("");
        } else {
            groups.push_back(subject.substr(start, end - start));
        }
    }

    pcre2_match_data_free(match_data);
    return rc;
}

} // namespace shield

#endif // SHIELD_REGEX_PCRE2
