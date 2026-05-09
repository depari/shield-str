// examples/basic_usage/main.cpp
// Quick Start: shield-str 5분 통합 예제

#include <shield/shield.hpp>
#include <iostream>
#include <variant>
#include <string_view>
#include <vector>

int main() {
    // 1. RuleManager 초기화 (JSON 문자열 또는 파일)
    const std::string RULES = R"json({
      "version": "1.0",
      "rules": [
        {
          "id": "rule_password",
          "trigger_keywords": ["password", "passwd"],
          "pattern": "(?i)(?:password|passwd)=(\\S+)",
          "mask_group": 1,
          "replacement": "***"
        },
        {
          "id": "rule_email",
          "trigger_keywords": ["@"],
          "pattern": "[a-zA-Z0-9._%+\\-]+@[a-zA-Z0-9.\\-]+\\.[a-zA-Z]{2,}",
          "mask_group": 0,
          "replacement": "***@***.***"
        },
        {
          "id": "rule_bearer",
          "trigger_keywords": ["Bearer"],
          "pattern": "Bearer\\s+([A-Za-z0-9\\-._~+/]+=*)",
          "mask_group": 1,
          "replacement": "***REDACTED***"
        }
      ]
    })json";

    auto mgr    = shield::RuleManager::from_json(RULES);
    auto engine = std::make_shared<shield::MaskingEngine>(mgr);

    // 2. 마스킹 처리
    std::vector<std::string_view> logs = {
        "user=alice action=login status=ok",
        "user=alice password=S3cr3t! action=login",
        "email: alice@example.com connected",
        "Authorization: Bearer eyJhbGciOiJSUzI1NiJ9.payload.sig",
        "user=bob passwd=hunter2 email=bob@corp.io",
    };

    std::cout << "=== shield-str Quick Start Demo ===" << std::endl << std::endl;
    for (const auto& log : logs) {
        auto result = engine->process(log);

        bool zero_copy = std::holds_alternative<std::string_view>(result);
        std::string output;
        if (zero_copy) {
            output = std::string(std::get<std::string_view>(result));
        } else {
            output = std::get<std::string>(result);
        }

        std::cout << "[INPUT ] " << log   << std::endl
                  << "[OUTPUT] " << output << std::endl
                  << "[MODE  ] " << (zero_copy ? "Zero-Copy" : "Masked") << std::endl
                  << "----" << std::endl;
    }

    // 3. 런타임 룰 업데이트 (Zero-Downtime)
    std::cout << std::endl << "=== Hot Rule Update Demo ===" << std::endl;
    const std::string NEW_RULES = R"json({
      "version": "2.0",
      "rules": [
        {
          "id": "rule_password",
          "trigger_keywords": ["password"],
          "pattern": "(?i)password=(\\S+)",
          "mask_group": 1,
          "replacement": "[PASSWORD REMOVED]"
        }
      ]
    })json";

    mgr->update_rules(NEW_RULES);

    auto r = engine->process("password=NewSecret user=alice");
    std::string out;
    if (std::holds_alternative<std::string>(r)) {
        out = std::get<std::string>(r);
    } else {
        out = std::string(std::get<std::string_view>(r));
    }
    std::cout << "After update: " << out << std::endl;

    return 0;
}
