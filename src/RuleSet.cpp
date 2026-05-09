// shield/RuleSet.cpp

#include "shield/RuleSet.hpp"
#include <re2/re2.h>
#include <stdexcept>

namespace shield {

RuleSet::RuleSet(std::vector<RuleDefinition> rules) {
    std::vector<std::string> all_keywords;

    for (auto& def : rules) {
        // Collect trigger keywords for AhoCorasick
        for (auto& kw : def.trigger_keywords) {
            if (!kw.empty()) {
                all_keywords.push_back(kw);
            }
        }

        // Compile RE2 pattern
        re2::RE2::Options opts;
        opts.set_log_errors(false);  // suppress RE2 stderr output

        auto compiled = std::make_unique<re2::RE2>(def.pattern_str, opts);
        if (!compiled->ok()) {
            // Skip invalid patterns (warning-level; RuleManager logs this)
            continue;
        }

        Re2PatternMatcher::Rule rule;
        rule.id          = std::move(def.id);
        rule.pattern     = std::move(compiled);
        rule.mask_group  = def.mask_group;
        rule.replacement = std::move(def.replacement);

        matcher_.add_rule(std::move(rule));
        ++rule_count_;
    }

    // Build Aho-Corasick automaton from all collected keywords
    scanner_.build(all_keywords);
}

} // namespace shield
