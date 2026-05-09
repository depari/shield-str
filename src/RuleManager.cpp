// shield/RuleManager.cpp

#include "shield/RuleManager.hpp"
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <shared_mutex>

#ifdef SHIELD_USE_STD_ONLY
#include <regex>
#else
#include "simdjson.h"
#endif

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

#ifdef SHIELD_USE_STD_ONLY
// A minimal ad-hoc JSON extractor for fallback mode
static std::string extract_json_string(std::string_view json, const std::string& key) {
    std::string s(json);
    std::string target = "\"" + key + "\"";
    size_t pos = s.find(target);
    if (pos == std::string::npos) return "";
    
    pos = s.find(":", pos + target.length());
    if (pos == std::string::npos) return "";
    
    pos = s.find("\"", pos + 1);
    if (pos == std::string::npos) return "";
    
    size_t start = pos + 1;
    std::string val;
    bool escape = false;
    for (size_t i = start; i < s.length(); ++i) {
        char c = s[i];
        if (escape) {
            val += "\\" + std::string(1, c);
            escape = false;
        } else if (c == '\\') {
            escape = true;
        } else if (c == '"') {
            break;
        } else {
            val += c;
        }
    }
    return val;
}

static int extract_json_int(std::string_view json, const std::string& key, int default_val) {
    std::string pattern = "\"" + key + "\"\\s*:\\s*(\\d+)";
    std::regex re(pattern);
    std::smatch match;
    std::string s(json);
    if (std::regex_search(s, match, re)) {
        return std::stoi(match[1].str());
    }
    return default_val;
}

static std::vector<std::string> extract_json_string_array(std::string_view json, const std::string& key) {
    std::vector<std::string> res;
    std::string pattern = "\"" + key + "\"\\s*:\\s*\\[(.*?)\\]";
    std::regex re(pattern);
    std::smatch match;
    std::string s(json);
    if (std::regex_search(s, match, re)) {
        std::string arr_str = match[1].str();
        std::regex elem_re("\"([^\"]*)\"");
        auto begin = std::sregex_iterator(arr_str.begin(), arr_str.end(), elem_re);
        auto end = std::sregex_iterator();
        for (auto i = begin; i != end; ++i) {
            res.push_back((*i)[1].str());
        }
    }
    return res;
}
#endif

// Parse JSON and produce a compiled RuleSet
std::shared_ptr<const RuleSet>
RuleManager::parse_and_compile(std::string_view json_str) {
    std::vector<RuleDefinition> defs;

#ifdef SHIELD_USE_STD_ONLY
    // Fallback Simplistic JSON parsing (Brace counting)
    std::string s(json_str);
    
    // Find the rules array first to avoid parsing the whole document
    size_t rules_start = s.find("\"rules\"");
    if (rules_start == std::string::npos) {
        throw RuleLoadError("JSON missing 'rules' array");
    }
    
    size_t pos = rules_start;
    while ((pos = s.find('{', pos)) != std::string::npos) {
        size_t start = pos;
        int brace_count = 0;
        bool in_string = false;
        bool escape = false;
        size_t end_pos = std::string::npos;
        for (size_t i = start; i < s.length(); ++i) {
            char c = s[i];
            if (escape) { escape = false; continue; }
            if (c == '\\') { escape = true; continue; }
            if (c == '"') { in_string = !in_string; continue; }
            if (!in_string) {
                if (c == '{') brace_count++;
                else if (c == '}') {
                    brace_count--;
                    if (brace_count == 0) {
                        end_pos = i;
                        break;
                    }
                }
            }
        }
        if (end_pos == std::string::npos) break;

        std::string rule_str = s.substr(start, end_pos - start + 1);
        RuleDefinition def;
        
        def.id = extract_json_string(rule_str, "id");
        if (def.id.empty()) { pos = end_pos + 1; continue; }
        
        def.description = extract_json_string(rule_str, "description");
        def.pattern_str = extract_json_string(rule_str, "pattern");
        if (def.pattern_str.empty()) { pos = end_pos + 1; continue; }

        // Fix escaped backslashes from JSON parsing
        std::string unescaped;
        for (size_t j = 0; j < def.pattern_str.length(); ++j) {
            if (def.pattern_str[j] == '\\' && j + 1 < def.pattern_str.length()) {
                if (def.pattern_str[j+1] == '\\') {
                    unescaped += '\\';
                    ++j;
                } else {
                    unescaped += def.pattern_str[j];
                }
            } else {
                unescaped += def.pattern_str[j];
            }
        }
        def.pattern_str = unescaped;

        def.mask_group = extract_json_int(rule_str, "mask_group", 0);
        
        def.replacement = extract_json_string(rule_str, "replacement");
        if (def.replacement.empty()) def.replacement = "***";

        def.trigger_keywords = extract_json_string_array(rule_str, "trigger_keywords");

        defs.push_back(std::move(def));
        pos = end_pos + 1;
    }
    
    if (defs.empty() && s.find("\"rules\"") == std::string::npos) {
        throw RuleLoadError("JSON missing 'rules' array or bad format");
    }

#else
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
#endif

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
