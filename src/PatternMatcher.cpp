// shield/PatternMatcher.cpp
#include "shield/PatternMatcher.hpp"
#include <algorithm>
#include <stdexcept>

namespace shield {

PatternMatcher::PatternMatcher() = default;
PatternMatcher::~PatternMatcher() = default;
PatternMatcher::PatternMatcher(PatternMatcher&&) noexcept = default;
PatternMatcher& PatternMatcher::operator=(PatternMatcher&&) noexcept = default;

void PatternMatcher::add_rule(Rule rule) {
    if (!rule.pattern) {
        throw std::invalid_argument("PatternMatcher: invalid pattern for rule '" + rule.id + "'");
    }
#ifndef SHIELD_REGEX_STD
    if (!rule.pattern->ok()) {
        throw std::invalid_argument("PatternMatcher: invalid pattern (compilation failed) for rule '" + rule.id + "'");
    }
#endif
    rules_.push_back(std::move(rule));
}

template <typename F>
static std::string format_replacement(const std::string& replacement, F&& get_group) {
    std::string result;
    result.reserve(replacement.size() * 2);
    for (std::size_t i = 0; i < replacement.size(); ++i) {
        if (replacement[i] == '$' && i + 1 < replacement.size() && std::isdigit(static_cast<unsigned char>(replacement[i+1]))) {
            int group_idx = replacement[i+1] - '0';
            result.append(get_group(group_idx));
            ++i; // skip digit
        } else {
            result.push_back(replacement[i]);
        }
    }
    return result;
}

std::optional<std::string> PatternMatcher::apply(std::string_view text) const {
    if (rules_.empty()) return std::nullopt;

    std::string result(text);
    bool        modified = false;

    for (const auto& rule : rules_) {
#ifdef SHIELD_REGEX_STD
        std::string pattern_str = result;
        std::string new_result;
        auto begin = std::sregex_iterator(pattern_str.begin(), pattern_str.end(), *rule.pattern);
        auto end = std::sregex_iterator();
        if (begin == end) continue;
        size_t last_pos = 0;
        for (auto i = begin; i != end; ++i) {
            std::smatch match = *i;
            new_result.append(pattern_str.substr(last_pos, match.position() - last_pos));
            std::string replacement = format_replacement(rule.replacement, [&](int idx) {
                return (idx < (int)match.size()) ? match[idx].str() : "";
            });
            if (rule.mask_group == 0) {
                new_result.append(replacement);
            } else if (rule.mask_group < (int)match.size()) {
                std::string full_match = match[0].str();
                std::string target = match[rule.mask_group].str();
                size_t target_pos = full_match.find(target);
                if (target_pos != std::string::npos) {
                    std::string substituted = full_match;
                    substituted.replace(target_pos, target.length(), replacement);
                    new_result.append(substituted);
                } else {
                    new_result.append(full_match);
                }
            } else {
                new_result.append(match[0].str());
            }
            last_pos = match.position() + match.length();
        }
        new_result.append(pattern_str.substr(last_pos));
        result = std::move(new_result);
        modified = true;

#elif defined(SHIELD_REGEX_PCRE2)
        std::string next_result;
        size_t last_consumed = 0;
        bool rule_matched = false;

        while (last_consumed <= result.size()) {
            std::vector<std::string_view> submatches;
            std::string_view current_subject = std::string_view(result).substr(last_consumed);
            int rc = rule.pattern->match_groups(current_subject, submatches);

            if (rc > 0) {
                std::string_view full_match = submatches[0];
                size_t relative_offset = full_match.data() - current_subject.data();
                size_t absolute_offset = last_consumed + relative_offset;
                
                next_result.append(result.substr(last_consumed, relative_offset));

                std::string replacement = format_replacement(rule.replacement, [&](int idx) {
                    return (idx < (int)submatches.size()) ? std::string(submatches[idx]) : "";
                });

                if (rule.mask_group == 0) {
                    next_result.append(replacement);
                } else if (rule.mask_group < (int)submatches.size()) {
                    std::string_view target = submatches[rule.mask_group];
                    size_t target_offset_in_match = target.data() - full_match.data();
                    next_result.append(std::string(full_match.substr(0, target_offset_in_match)));
                    next_result.append(replacement);
                    next_result.append(std::string(full_match.substr(target_offset_in_match + target.size())));
                } else {
                    next_result.append(std::string(full_match));
                }
                
                last_consumed = absolute_offset + full_match.size();
                rule_matched = true;
                
                if (full_match.empty()) {
                    if (last_consumed < result.size()) {
                        next_result.push_back(result[last_consumed]);
                        last_consumed++;
                    } else {
                        break;
                    }
                }
            } else {
                next_result.append(result.substr(last_consumed));
                break;
            }
        }
        if (rule_matched) {
            result = std::move(next_result);
            modified = true;
        }

#else
        // RE2
        std::string next_result;
        size_t last_consumed = 0;
        bool rule_matched = false;
        int n = rule.pattern->NumberOfCapturingGroups() + 1;
        std::vector<re2::StringPiece> groups(n);

        while (last_consumed <= result.size()) {
            re2::StringPiece subject(result.data() + last_consumed, result.size() - last_consumed);
            if (rule.pattern->Match(re2::StringPiece(result), last_consumed, result.size(), re2::RE2::UNANCHORED, groups.data(), n)) {
                size_t match_start_abs = groups[0].data() - result.data();
                next_result.append(result.substr(last_consumed, match_start_abs - last_consumed));

                std::string replacement = format_replacement(rule.replacement, [&](int idx) {
                    return (idx < n) ? std::string(groups[idx]) : "";
                });

                if (rule.mask_group == 0) {
                    next_result.append(replacement);
                } else if (rule.mask_group < n) {
                    re2::StringPiece target = groups[rule.mask_group];
                    size_t target_offset_in_match = target.data() - groups[0].data();
                    next_result.append(std::string(groups[0].substr(0, target_offset_in_match)));
                    next_result.append(replacement);
                    next_result.append(std::string(groups[0].substr(target_offset_in_match + target.size())));
                } else {
                    next_result.append(std::string(groups[0]));
                }
                
                last_consumed = match_start_abs + groups[0].size();
                rule_matched = true;
                
                if (groups[0].empty()) {
                    if (last_consumed < result.size()) {
                        next_result.push_back(result[last_consumed]);
                        last_consumed++;
                    } else {
                        break;
                    }
                }
            } else {
                next_result.append(result.substr(last_consumed));
                break;
            }
        }

        if (rule_matched) {
            result = std::move(next_result);
            modified = true;
        }
#endif
    }

    return modified ? std::make_optional(result) : std::nullopt;
}

} // namespace shield
