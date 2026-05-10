// shield/PatternMatcher.hpp
#pragma once
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#ifdef SHIELD_REGEX_STD
#include <regex>
#elif defined(SHIELD_REGEX_PCRE2)
#include "../../src/PCRE2Wrapper.hpp"
#else
#include <re2/re2.h>
#endif

namespace shield {

/**
 * @brief Thread-safe pattern matcher for multiple regex engines.
 */
class PatternMatcher {
public:
    struct Rule {
        std::string              id;
        
#ifdef SHIELD_REGEX_STD
        std::unique_ptr<std::regex> pattern;
#elif defined(SHIELD_REGEX_PCRE2)
        std::unique_ptr<shield::PCRE2Regex> pattern;
#else
        std::unique_ptr<re2::RE2> pattern;
#endif
        int                       mask_group;
        std::string              replacement;
        std::vector<std::string> trigger_keywords; // Added to help RuleSet
    };

    PatternMatcher();
    ~PatternMatcher();

    PatternMatcher(const PatternMatcher&)            = delete;
    PatternMatcher& operator=(const PatternMatcher&) = delete;

    PatternMatcher(PatternMatcher&&)            noexcept;
    PatternMatcher& operator=(PatternMatcher&&) noexcept;

    /**
     * @brief Add a rule to the matcher.
     */
    void add_rule(Rule rule);

    /**
     * @brief Apply all rules to the input text and return the redacted result.
     *        If no rules match, returns nullopt.
     */
    [[nodiscard]] std::optional<std::string> apply(std::string_view text) const;

    [[nodiscard]] bool empty() const noexcept { return rules_.empty(); }
    [[nodiscard]] const std::vector<Rule>& rules() const noexcept { return rules_; }

private:
    std::vector<Rule> rules_;
};

} // namespace shield
