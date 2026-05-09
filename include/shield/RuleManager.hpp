#pragma once
// shield/RuleManager.hpp — Rule lifecycle & RCU-style lock-free RuleSet swap

#include "shield/RuleSet.hpp"
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <string_view>
#include <stdexcept>

namespace shield {

/**
 * @brief Thrown when a rule configuration cannot be parsed or compiled.
 */
class RuleLoadError : public std::runtime_error {
    using std::runtime_error::runtime_error;
};

/**
 * @brief Manages the lifecycle of RuleSet objects.
 *
 * Implements a Read-Copy-Update (RCU) style pattern using
 * shared_mutex: readers hold shared lock (many concurrent readers),
 * writer holds exclusive lock only during pointer swap (microseconds).
 *
 * This provides:
 *  - Zero blocking for readers in normal operation
 *  - Lock-free semantics via shared_ptr copy-on-read
 *  - Safe hot-swap without application restart
 */
class RuleManager {
public:
    RuleManager() = default;

    /**
     * @brief Load rules from a JSON file path.
     * @throws RuleLoadError  If the file cannot be read or parsed.
     */
    static std::shared_ptr<RuleManager> from_file(std::string_view path);

    /**
     * @brief Load rules from a JSON string.
     * @throws RuleLoadError  If the JSON cannot be parsed.
     */
    static std::shared_ptr<RuleManager> from_json(std::string_view json_str);

    /**
     * @brief Hot-swap the current rule set.
     *        Acquires exclusive lock only during pointer swap (~nanoseconds).
     * @throws RuleLoadError  On parse or compile failure (old rules preserved).
     */
    void update_rules(std::string_view json_config);

    /**
     * @brief Get the current rule set (shared ownership).
     *        Acquires shared lock briefly to copy the shared_ptr.
     */
    [[nodiscard]]
    std::shared_ptr<const RuleSet> get_current_ruleset() const;

private:
    // Parse JSON and compile into a new RuleSet
    static std::shared_ptr<const RuleSet> parse_and_compile(std::string_view json_str);

    // RCU-style: shared_mutex protects pointer swap only
    mutable std::shared_mutex              mutex_;
    std::shared_ptr<const RuleSet>         current_ruleset_;
};

} // namespace shield
