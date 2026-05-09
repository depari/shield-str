#pragma once
// shield/PatternMatcher.hpp — RE2-based pattern matching & masking

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#ifdef SHIELD_USE_STD_ONLY
#include <regex>
#else
#include <re2/re2.h>
#endif

namespace shield {

/**
 * @brief Holds compiled RE2 rules and applies masking to input text.
 *
 * Each rule specifies:
 *  - A compiled RE2 pattern.
 *  - Which capture group to replace (0 = entire match).
 *  - The replacement string.
 */
class PatternMatcher {
public:
    struct Rule {
        std::string              id;
#ifdef SHIELD_USE_STD_ONLY
        std::unique_ptr<std::regex> pattern;
#else
        std::unique_ptr<re2::RE2> pattern;
#endif
        int                       mask_group;   ///< Capture group index to redact (0 = full match)
        std::string              replacement;
    };

    PatternMatcher()  = default;
    ~PatternMatcher() = default;

    // Non-copyable (RE2 objects are not copyable)
    PatternMatcher(const PatternMatcher&)            = delete;
    PatternMatcher& operator=(const PatternMatcher&) = delete;

    PatternMatcher(PatternMatcher&&)            = default;
    PatternMatcher& operator=(PatternMatcher&&) = default;

    /**
     * @brief Add a compiled rule.  Takes ownership of the RE2 object.
     */
    void add_rule(Rule rule);

    /**
     * @brief Apply all rules to the input text.
     * @return A new std::string with all matches redacted,
     *         or std::nullopt if no rule matched.
     */
    [[nodiscard]] std::optional<std::string> apply(std::string_view text) const;

    [[nodiscard]] bool empty() const noexcept { return rules_.empty(); }
    [[nodiscard]] std::size_t size() const noexcept { return rules_.size(); }

private:
    std::vector<Rule> rules_;
};

} // namespace shield
