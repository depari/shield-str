// shield/RuleManager.cpp
#include "shield/RuleManager.hpp"
#include "shield/RuleSet.hpp"
#include <fstream>
#include <sstream>
#include <regex>
#include <algorithm>

#ifndef SHIELD_USE_STD_ONLY
#include "simdjson.h"
#endif

namespace shield {

static std::string read_file(std::string_view path) {
    std::ifstream ifs{std::string(path)};
    if (!ifs.is_open()) {
        throw RuleLoadError("Failed to open file: " + std::string(path));
    }
    std::stringstream ss;
    ss << ifs.rdbuf();
    return ss.str();
}

static std::string unescape_json_string(std::string_view s) {
    std::string res;
    res.reserve(s.size());
    for (size_t i = 0; i < s.size(); ++i) {
        if (s[i] == '\\' && i + 1 < s.size()) {
            switch (s[i+1]) {
                case '"':  res += '"';  i++; break;
                case '\\': res += '\\'; i++; break;
                case '/':  res += '/';  i++; break;
                case 'b':  res += '\b'; i++; break;
                case 'f':  res += '\f'; i++; break;
                case 'n':  res += '\n'; i++; break;
                case 'r':  res += '\r'; i++; break;
                case 't':  res += '\t'; i++; break;
                default:   res += s[i];      break; // keep backslash
            }
        } else {
            res += s[i];
        }
    }
    return res;
}

#ifdef SHIELD_USE_STD_ONLY
static std::string trim(const std::string& s) {
    auto start = s.find_first_not_of(" \t\n\r");
    if (start == std::string::npos) return "";
    auto end = s.find_last_not_of(" \t\n\r");
    return s.substr(start, end - start + 1);
}

static std::string extract_val(const std::string& json, const std::string& key) {
    std::string k = "\"" + key + "\"";
    size_t pos = json.find(k);
    if (pos == std::string::npos) return "";
    size_t colon = json.find(":", pos);
    if (colon == std::string::npos) return "";
    return json.substr(colon + 1);
}

static std::string extract_json_string(const std::string& json, const std::string& key) {
    std::string val = extract_val(json, key);
    size_t start = val.find("\"");
    if (start == std::string::npos) return "";
    size_t end = val.find("\"", start + 1);
    if (end == std::string::npos) return "";
    return val.substr(start + 1, end - start - 1);
}

static int extract_json_int(const std::string& json, const std::string& key, int default_val) {
    std::string val = trim(extract_val(json, key));
    if (val.empty()) return default_val;
    std::string digits;
    for (char c : val) {
        if (std::isdigit(c)) digits += c;
        else if (!digits.empty()) break;
    }
    return digits.empty() ? default_val : std::stoi(digits);
}

static std::vector<std::string> extract_json_string_array(const std::string& json, const std::string& key) {
    std::vector<std::string> res;
    std::string val = extract_val(json, key);
    size_t start = val.find("[");
    if (start == std::string::npos) return res;
    size_t end = val.find("]", start + 1);
    if (end == std::string::npos) return res;
    
    std::string arr_str = val.substr(start + 1, end - start - 1);
    size_t q_pos = 0;
    while ((q_pos = arr_str.find("\"", q_pos)) != std::string::npos) {
        size_t q_end = arr_str.find("\"", q_pos + 1);
        if (q_end == std::string::npos) break;
        res.push_back(unescape_json_string(arr_str.substr(q_pos + 1, q_end - q_pos - 1)));
        q_pos = q_end + 1;
    }
    return res;
}
#endif

std::shared_ptr<const RuleSet>
RuleManager::parse_and_compile(std::string_view json_str) {
    std::vector<RuleDefinition> defs;

#ifdef SHIELD_USE_STD_ONLY
    std::string s(json_str);
    size_t rules_pos = s.find("\"rules\"");
    if (rules_pos == std::string::npos) {
        throw RuleLoadError("JSON missing 'rules' array");
    }
    
    size_t start_bracket = s.find("[", rules_pos);
    if (start_bracket == std::string::npos) throw RuleLoadError("Invalid JSON");
    
    size_t end_bracket = s.find_last_of("]");
    if (end_bracket == std::string::npos || end_bracket < start_bracket) throw RuleLoadError("Invalid JSON");
    
    std::string rules_content = s.substr(start_bracket + 1, end_bracket - start_bracket - 1);
    
    size_t pos = 0;
    while ((pos = rules_content.find("{", pos)) != std::string::npos) {
        // Simple brace counting to find end of object
        int brace_count = 1;
        size_t end_obj = pos + 1;
        while (brace_count > 0 && end_obj < rules_content.size()) {
            if (rules_content[end_obj] == '{') brace_count++;
            else if (rules_content[end_obj] == '}') brace_count--;
            end_obj++;
        }
        
        if (brace_count == 0) {
            std::string rule_str = rules_content.substr(pos, end_obj - pos);
            RuleDefinition def;
            def.id = extract_json_string(rule_str, "id");
            if (!def.id.empty()) {
                def.description = extract_json_string(rule_str, "description");
                def.pattern_str = unescape_json_string(extract_json_string(rule_str, "pattern"));
                def.mask_group = extract_json_int(rule_str, "mask_group", 0);
                def.replacement = unescape_json_string(extract_json_string(rule_str, "replacement"));
                if (def.replacement.empty()) def.replacement = "***";
                def.trigger_keywords = extract_json_string_array(rule_str, "trigger_keywords");
                
                if (!def.pattern_str.empty()) {
                    defs.push_back(std::move(def));
                }
            }
            pos = end_obj;
        } else {
            break;
        }
    }
#else
    simdjson::padded_string padded(json_str.data(), json_str.size());
    simdjson::ondemand::parser parser;
    simdjson::ondemand::document doc;
    auto err = parser.iterate(padded).get(doc);
    if (err) {
        throw RuleLoadError(std::string("JSON parse error: ") + simdjson::error_message(err));
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

        std::string_view sv;
        if (rule_obj["id"].get(sv) == simdjson::SUCCESS) def.id = std::string(sv);
        if (def.id.empty()) continue;

        if (rule_obj["description"].get(sv) == simdjson::SUCCESS) def.description = std::string(sv);

        simdjson::ondemand::array kw_arr;
        if (rule_obj["trigger_keywords"].get(kw_arr) == simdjson::SUCCESS) {
            for (auto kw_val : kw_arr) {
                if (kw_val.get(sv) == simdjson::SUCCESS && !sv.empty()) {
                    def.trigger_keywords.emplace_back(unescape_json_string(sv));
                }
            }
        }

        if (rule_obj["pattern"].get(sv) == simdjson::SUCCESS) def.pattern_str = unescape_json_string(sv);
        if (def.pattern_str.empty()) continue;

        int64_t mg = 0;
        if (rule_obj["mask_group"].get(mg) == simdjson::SUCCESS) def.mask_group = static_cast<int>(mg);

        def.replacement = "***";
        if (rule_obj["replacement"].get(sv) == simdjson::SUCCESS) def.replacement = unescape_json_string(sv);

        defs.push_back(std::move(def));
    }
#endif

    return std::make_shared<const RuleSet>(std::move(defs));
}

std::shared_ptr<RuleManager> RuleManager::from_file(std::string_view path) {
    auto json_str = read_file(path);
    auto mgr      = std::make_shared<RuleManager>();
    mgr->update_rules(json_str);
    return mgr;
}

std::shared_ptr<RuleManager> RuleManager::from_json(std::string_view json_str) {
    auto mgr = std::make_shared<RuleManager>();
    mgr->update_rules(json_str);
    return mgr;
}

std::shared_ptr<const RuleSet> RuleManager::get_current_ruleset() const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    if (!current_ruleset_) {
        return std::make_shared<const RuleSet>(std::vector<RuleDefinition>{});
    }
    return current_ruleset_;
}

void RuleManager::update_rules(std::string_view json_config) {
    auto new_ruleset = parse_and_compile(json_config);
    std::unique_lock<std::shared_mutex> lock(mutex_);
    current_ruleset_ = std::move(new_ruleset);
}

} // namespace shield
