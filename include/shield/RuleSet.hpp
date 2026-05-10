#pragma once
// shield/RuleSet.hpp — Immutable, pre-compiled rule container

#include "shield/AhoCorasickScanner.hpp"
#include "shield/PatternMatcher.hpp"
#include <string>
#include <vector>

namespace shield {

/**
 * @brief Parsed definition of a single masking rule (loaded from JSON).
 */
struct RuleDefinition {
    std::string              id;
    std::string              description;
    std::vector<std::string> trigger_keywords;
    std::string              pattern_str;
    int                      mask_group   = 0;
    std::string              replacement  = "***";
};

/**
 * @brief Immutable, thread-safe compiled rule set.
 *
 * Once constructed, this object is read-only and can be shared safely
 * across threads via std::shared_ptr<const RuleSet>.
 */
class RuleSet {
public:
    /**
     * @brief Compile all rules into the internal scanner and matcher.
     * @param rules  Parsed rule definitions.
     */
    explicit RuleSet(std::vector<RuleDefinition> rules);

    ~RuleSet() = default;

    // Non-copyable / non-movable (immutable value semantics via shared_ptr)
    RuleSet(const RuleSet&)            = delete;
    RuleSet& operator=(const RuleSet&) = delete;

    [[nodiscard]] const AhoCorasickScanner&  scanner() const noexcept { return scanner_; }
    [[nodiscard]] const PatternMatcher&   matcher() const noexcept { return matcher_; }
    [[nodiscard]] std::size_t rule_count() const noexcept;

private:
    AhoCorasickScanner scanner_;
    PatternMatcher  matcher_;
    std::size_t        rule_count_ = 0;
};

} // namespace shield
