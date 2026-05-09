// shield/MaskingEngine.cpp

#include "shield/MaskingEngine.hpp"

namespace shield {

MaskingEngine::MaskingEngine(std::shared_ptr<RuleManager> rule_manager)
    : rule_manager_(std::move(rule_manager)) {
    if (!rule_manager_) {
        throw std::invalid_argument("MaskingEngine: rule_manager must not be null");
    }
}

MaskingEngine::Result MaskingEngine::process(std::string_view input) const {
    if (input.empty()) return input;  // Zero-Copy: return original string_view

    auto ruleset = rule_manager_->get_current_ruleset();
    if (!ruleset) return input;       // No rules loaded yet

    // ── Fast path: Aho-Corasick keyword presence check ──────────────────────
    if (!ruleset->scanner().scan(input)) {
        return input;  // Zero-Copy: no trigger keyword found
    }

    // ── Slow path: RE2 pattern matching & replacement ────────────────────────
    auto masked = ruleset->matcher().apply(input);
    if (!masked.has_value()) {
        // Keywords found but patterns didn't match (false trigger) → Zero-Copy
        return input;
    }
    return std::move(*masked);
}

} // namespace shield
