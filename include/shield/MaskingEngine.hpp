#pragma once
// shield/MaskingEngine.hpp — Primary entry point for log masking

#include "shield/RuleManager.hpp"
#include <memory>
#include <string>
#include <string_view>
#include <variant>

namespace shield {

/**
 * @brief High-performance log masking engine.
 *
 * Fast path (no trigger keywords found):
 *   Returns the original std::string_view — zero allocation, zero copy.
 *
 * Slow path (keyword detected → RE2 matching):
 *   Returns a new std::string with sensitive data replaced.
 *
 * Example usage:
 * @code
 *   auto mgr    = shield::RuleManager::from_file("rules/default_rules.json");
 *   auto engine = std::make_shared<shield::MaskingEngine>(mgr);
 *
 *   auto result = engine->process("password=secret123 user=alice");
 *   // result holds std::string "password=*** user=alice"
 * @endcode
 */
class MaskingEngine {
public:
    /**
     * @brief Result type.
     *  - std::string_view : original input, no masking needed (Zero-Copy)
     *  - std::string      : new buffer with sensitive data replaced
     */
    using Result = std::variant<std::string_view, std::string>;

    explicit MaskingEngine(std::shared_ptr<RuleManager> rule_manager);

    /**
     * @brief Process a log string through the masking pipeline.
     *
     * @param input  The log message to inspect (must outlive the Result
     *               if the returned variant holds string_view).
     * @return Result  See Result typedef above.
     */
    [[nodiscard]] Result process(std::string_view input) const;

private:
    std::shared_ptr<RuleManager> rule_manager_;
};

} // namespace shield
