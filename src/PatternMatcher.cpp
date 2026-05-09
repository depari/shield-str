// shield/PatternMatcher.cpp

#include "shield/PatternMatcher.hpp"
#include <stdexcept>

#ifdef SHIELD_USE_STD_ONLY
#include <regex>
#else
#include <re2/re2.h>
#endif

namespace shield {

void PatternMatcher::add_rule(Rule rule) {
    if (!rule.pattern) {
        throw std::invalid_argument(
            "PatternMatcher: invalid pattern for rule '" + rule.id + "'");
    }
#ifndef SHIELD_USE_STD_ONLY
    if (!rule.pattern->ok()) {
        throw std::invalid_argument(
            "PatternMatcher: invalid RE2 pattern for rule '" + rule.id + "'");
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
#ifdef SHIELD_USE_STD_ONLY
        if (!rule.pattern) continue;
        
        std::string buf;
        std::sregex_iterator it(result.begin(), result.end(), *rule.pattern);
        std::sregex_iterator end;

        if (it == end) continue; // No match

        std::size_t last_pos = 0;
        const int group = rule.mask_group;

        for (; it != end; ++it) {
            std::smatch match = *it;
            if (match.empty()) continue;

            buf.append(result, last_pos, match.position() - last_pos);

            auto get_group = [&](int idx) -> std::string {
                if (static_cast<std::size_t>(idx) < match.size() && match[idx].matched) {
                    return match[idx].str();
                }
                return "";
            };
            std::string formatted = format_replacement(rule.replacement, get_group);

            if (group == 0 || static_cast<std::size_t>(group) >= match.size()) {
                buf.append(formatted);
            } else {
                auto sub = match[group];
                if (sub.matched) {
                    std::size_t full_start = match.position();
                    std::size_t target_start = match.position(group);
                    
                    buf.append(result, full_start, target_start - full_start);
                    buf.append(formatted);
                    std::size_t target_end = target_start + sub.length();
                    buf.append(result, target_end, match.length() - (target_end - full_start));
                } else {
                    buf.append(match.str());
                }
            }
            last_pos = match.position() + match.length();
            modified = true;
        }
        buf.append(result, last_pos, std::string::npos);
        result = std::move(buf);
#else
        if (!rule.pattern || !rule.pattern->ok()) continue;

        const int num_groups = rule.pattern->NumberOfCapturingGroups() + 1;
        const int group      = rule.mask_group;

        if (group < 0 || group >= num_groups) continue;

        std::string buf;
        buf.reserve(result.size());

        re2::StringPiece input(result);
        std::vector<re2::StringPiece> submatch(static_cast<std::size_t>(num_groups));

        while (rule.pattern->Match(input, 0, input.size(),
                                    re2::RE2::UNANCHORED,
                                    submatch.data(),
                                    static_cast<int>(submatch.size()))) {
            const auto& full_match = submatch[0];
            const auto& target     = submatch[static_cast<std::size_t>(group)];

            if (target.empty()) {
                if (input.empty()) break;
                buf.push_back(input[0]);
                input.remove_prefix(1);
                continue;
            }

            buf.append(input.data(),
                       static_cast<std::size_t>(full_match.data() - input.data()));

            auto get_group = [&](int idx) -> std::string_view {
                if (idx < num_groups) {
                    return std::string_view(submatch[idx].data(), submatch[idx].size());
                }
                return "";
            };
            std::string formatted = format_replacement(rule.replacement, get_group);

            if (group == 0) {
                buf.append(formatted);
            } else {
                const char* full_start   = full_match.data();
                const char* target_start = target.data();
                const char* target_end   = target.data() + target.size();
                const char* full_end     = full_match.data() + full_match.size();

                buf.append(full_start,
                           static_cast<std::size_t>(target_start - full_start));
                buf.append(formatted);
                buf.append(target_end,
                           static_cast<std::size_t>(full_end - target_end));
            }

            const std::size_t consumed =
                static_cast<std::size_t>(full_match.data() - input.data())
                + full_match.size();
            input.remove_prefix(consumed);
            modified = true;
        }

        buf.append(input.data(), input.size());
        result = std::move(buf);
#endif
    }

    if (!modified) return std::nullopt;
    return result;
}

} // namespace shield
