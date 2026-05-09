// shield/RuleSet.cpp

#include "shield/RuleSet.hpp"
#include <stdexcept>

#ifdef SHIELD_USE_STD_ONLY
#include <regex>
#else
#include <re2/re2.h>
#endif

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

        PatternMatcher::Rule rule;
        rule.id          = std::move(def.id);
        rule.mask_group  = def.mask_group;
        rule.replacement = std::move(def.replacement);

#ifdef SHIELD_USE_STD_ONLY
        try {
            auto flags = std::regex_constants::ECMAScript;
            std::string pat = def.pattern_str;
            if (pat.find("(?i)") == 0) {
                flags |= std::regex_constants::icase;
                pat = pat.substr(4);
            }
            auto compiled = std::make_unique<std::regex>(pat, flags);
            rule.pattern = std::move(compiled);
        } catch (const std::regex_error&) {
            // Skip invalid patterns
            continue;
        }
#else
        // Compile RE2 pattern
        re2::RE2::Options opts;
        opts.set_log_errors(false);  // suppress RE2 stderr output

        auto compiled = std::make_unique<re2::RE2>(def.pattern_str, opts);
        if (!compiled->ok()) {
            // Skip invalid patterns
            continue;
        }
        rule.pattern = std::move(compiled);
#endif

        matcher_.add_rule(std::move(rule));
        ++rule_count_;
    }

    // Build Aho-Corasick automaton from all collected keywords
    scanner_.build(all_keywords);
}

} // namespace shield
