// shield/Re2PatternMatcher.cpp

#include "shield/Re2PatternMatcher.hpp"
#include <re2/re2.h>
#include <stdexcept>

namespace shield {

void Re2PatternMatcher::add_rule(Rule rule) {
    if (!rule.pattern || !rule.pattern->ok()) {
        throw std::invalid_argument(
            "Re2PatternMatcher: invalid RE2 pattern for rule '" + rule.id + "'");
    }
    rules_.push_back(std::move(rule));
}

std::optional<std::string> Re2PatternMatcher::apply(std::string_view text) const {
    if (rules_.empty()) return std::nullopt;

    std::string result(text);
    bool        modified = false;

    for (const auto& rule : rules_) {
        if (!rule.pattern || !rule.pattern->ok()) continue;

        const int num_groups = rule.pattern->NumberOfCapturingGroups() + 1;
        const int group      = rule.mask_group;

        if (group < 0 || group >= num_groups) continue;

        // Use RE2::GlobalReplace-style loop with StringPiece
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
                // Avoid infinite loop on zero-length match
                if (input.empty()) break;
                buf.push_back(input[0]);
                input.remove_prefix(1);
                continue;
            }

            // Append everything before the full match
            buf.append(input.data(),
                       static_cast<std::size_t>(full_match.data() - input.data()));

            if (group == 0) {
                // Replace entire match
                buf.append(rule.replacement);
            } else {
                // Replace only the capture group; keep surrounding parts of full match
                const char* full_start   = full_match.data();
                const char* target_start = target.data();
                const char* target_end   = target.data() + target.size();
                const char* full_end     = full_match.data() + full_match.size();

                buf.append(full_start,
                           static_cast<std::size_t>(target_start - full_start));
                buf.append(rule.replacement);
                buf.append(target_end,
                           static_cast<std::size_t>(full_end - target_end));
            }

            const std::size_t consumed =
                static_cast<std::size_t>(full_match.data() - input.data())
                + full_match.size();
            input.remove_prefix(consumed);
            modified = true;
        }

        // Append remainder
        buf.append(input.data(), input.size());
        result = std::move(buf);
    }

    if (!modified) return std::nullopt;
    return result;
}

} // namespace shield
