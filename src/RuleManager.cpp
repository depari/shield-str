// shield/RuleManager.cpp

#include "shield/RuleManager.hpp"
#include "simdjson.h"
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <shared_mutex>

namespace shield {

// ── helpers ──────────────────────────────────────────────────────────────────

static std::string read_file(std::string_view path) {
    std::ifstream ifs(std::string(path), std::ios::in | std::ios::binary);
    if (!ifs.is_open()) {
        throw RuleLoadError("Cannot open rule file: " + std::string(path));
    }
    std::ostringstream ss;
    ss << ifs.rdbuf();
    return ss.str();
}

// Parse JSON and produce a compiled RuleSet
std::shared_ptr<const RuleSet>
RuleManager::parse_and_compile(std::string_view json_str) {
    // simdjson requires the input to be padded
    simdjson::padded_string padded(json_str.data(), json_str.size());
    simdjson::ondemand::parser parser;

    simdjson::ondemand::document doc;
    auto err = parser.iterate(padded).get(doc);
    if (err) {
        throw RuleLoadError(
            std::string("JSON parse error: ") + simdjson::error_message(err));
    }

    simdjson::ondemand::array rules_arr;
    err = doc["rules"].get(rules_arr);
    if (err) {
        throw RuleLoadError("JSON missing 'rules' array");
    }

    std::vector<RuleDefinition> defs;

    for (auto rule_val : rules_arr) {
        RuleDefinition def;

        simdjson::ondemand::object rule_obj;
        if (rule_val.get(rule_obj) != simdjson::SUCCESS) continue;

        // id (required)
        std::string_view sv;
        if (rule_obj["id"].get(sv) != simdjson::SUCCESS) continue;
        def.id = std::string(sv);

        // description (optional)
        if (rule_obj["description"].get(sv) == simdjson::SUCCESS) {
            def.description = std::string(sv);
        }

        // trigger_keywords (required)
        simdjson::ondemand::array kw_arr;
        if (rule_obj["trigger_keywords"].get(kw_arr) != simdjson::SUCCESS) continue;
        for (auto kw_val : kw_arr) {
            if (kw_val.get(sv) == simdjson::SUCCESS && !sv.empty()) {
                def.trigger_keywords.emplace_back(sv);
            }
        }

        // pattern (required)
        if (rule_obj["pattern"].get(sv) != simdjson::SUCCESS) continue;
        def.pattern_str = std::string(sv);

        // mask_group (optional, default 0)
        int64_t mg = 0;
        if (rule_obj["mask_group"].get(mg) == simdjson::SUCCESS) {
            def.mask_group = static_cast<int>(mg);
        }

        // replacement (optional, default "***")
        def.replacement = "***";
        if (rule_obj["replacement"].get(sv) == simdjson::SUCCESS) {
            def.replacement = std::string(sv);
        }

        defs.push_back(std::move(def));
    }

    return std::make_shared<const RuleSet>(std::move(defs));
}

// ── Factory methods ───────────────────────────────────────────────────────────

std::shared_ptr<RuleManager>
RuleManager::from_file(std::string_view path) {
    auto json_str = read_file(path);
    auto mgr      = std::make_shared<RuleManager>();
    mgr->update_rules(json_str);
    return mgr;
}

std::shared_ptr<RuleManager>
RuleManager::from_json(std::string_view json_str) {
    auto mgr = std::make_shared<RuleManager>();
    mgr->update_rules(json_str);
    return mgr;
}

// ── Runtime update (RCU-style) ────────────────────────────────────────────────

void RuleManager::update_rules(std::string_view json_config) {
    // Build new RuleSet outside the lock (expensive compilation)
    auto new_ruleset = parse_and_compile(json_config);

    // Swap under exclusive lock (very fast — just pointer swap)
    std::unique_lock<std::shared_mutex> lock(mutex_);
    current_ruleset_ = std::move(new_ruleset);
}

std::shared_ptr<const RuleSet>
RuleManager::get_current_ruleset() const {
    // Shared lock: many readers can call this concurrently
    std::shared_lock<std::shared_mutex> lock(mutex_);
    return current_ruleset_;
}

} // namespace shield
