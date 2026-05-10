// shield/RuleSet.cpp
#include "shield/RuleSet.hpp"
#include <iostream>

namespace shield {

RuleSet::RuleSet(std::vector<RuleDefinition> rules) {
    for (auto& def : rules) {
        PatternMatcher::Rule rule;
        rule.id = def.id;
        rule.replacement = def.replacement;
        rule.mask_group = def.mask_group;
        rule.trigger_keywords = def.trigger_keywords;

#ifdef SHIELD_REGEX_STD
        std::regex_constants::syntax_option_type flags = std::regex_constants::ECMAScript;
        std::string pattern_str = def.pattern_str;
        if (pattern_str.find("(?i)") == 0) {
            flags |= std::regex_constants::icase;
            pattern_str = pattern_str.substr(4);
        }
        try {
            rule.pattern = std::make_unique<std::regex>(pattern_str, flags);
            matcher_.add_rule(std::move(rule));
            rule_count_++;
        } catch (const std::regex_error& e) {
            // std::cerr << "Regex error for rule " << def.id << ": " << e.what() << " pattern: " << pattern_str << std::endl;
        }
#elif defined(SHIELD_REGEX_PCRE2)
        auto compiled = std::make_unique<PCRE2Regex>(def.pattern_str);
        if (compiled->ok()) {
            rule.pattern = std::move(compiled);
            matcher_.add_rule(std::move(rule));
            rule_count_++;
        }
#else
        re2::RE2::Options opts;
        opts.set_log_errors(false);
        auto compiled = std::make_unique<re2::RE2>(def.pattern_str, opts);
        if (compiled->ok()) {
            rule.pattern = std::move(compiled);
            matcher_.add_rule(std::move(rule));
            rule_count_++;
        }
#endif
    }

    // Collect all trigger keywords for Aho-Corasick
    std::vector<std::string> all_keywords;
    for (const auto& r : matcher_.rules()) {
        for (const auto& kw : r.trigger_keywords) {
            if (!kw.empty()) all_keywords.push_back(kw);
        }
    }
    if (!all_keywords.empty()) {
        scanner_.build(all_keywords);
    }
}

std::size_t RuleSet::rule_count() const noexcept {
    return rule_count_;
}

} // namespace shield
