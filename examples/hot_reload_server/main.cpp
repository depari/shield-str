#include "shield/RuleManager.hpp"
#include "shield/MaskingEngine.hpp"
#include "httplib.h"
#include <iostream>
#include <string>
#include <thread>
#include <chrono>

using namespace shield;

int main() {
    // 1. 초기 룰 매니저 및 엔진 생성 (빈 룰셋으로 시작)
    auto mgr = RuleManager::from_json(R"({"version":"1.0","rules":[]})");
    MaskingEngine engine(mgr);

    // 2. 백그라운드 테스트용 로그 스트림 쓰레드
    std::thread log_thread([&engine]() {
        int i = 0;
        while (true) {
            std::this_thread::sleep_for(std::chrono::seconds(2));
            std::string log = "Log entry " + std::to_string(++i) + ": user login attempt password=secret_pw123!";
            auto masked = engine.process(log);
            if (std::holds_alternative<std::string>(masked)) {
                std::cout << "[App Log] " << std::get<std::string>(masked) << std::endl;
            } else {
                std::cout << "[App Log] " << std::get<std::string_view>(masked) << std::endl;
            }
        }
    });

    // 3. HTTP 서버 설정
    httplib::Server svr;

    // 현재 적용된 룰셋 확인 API
    svr.Get("/api/rules", [&mgr](const httplib::Request&, httplib::Response& res) {
        auto ruleset = mgr->get_current_ruleset();
        res.set_content("{\"status\":\"ok\", \"rules_count\": " + std::to_string(ruleset->rule_count()) + "}", "application/json");
    });

    // 동적 룰셋 업데이트 API
    svr.Post("/api/rules", [&mgr](const httplib::Request& req, httplib::Response& res) {
        try {
            mgr->update_rules(req.body);
            std::cout << "[Admin] Rules updated successfully." << std::endl;
            res.set_content("{\"status\":\"success\", \"message\":\"Rules updated successfully.\"}", "application/json");
        } catch (const std::exception& e) {
            std::cerr << "[Admin] Failed to update rules: " << e.what() << std::endl;
            res.status = 400;
            res.set_content("{\"status\":\"error\", \"message\":\"" + std::string(e.what()) + "\"}", "application/json");
        }
    });

    std::cout << "========================================\n";
    std::cout << "Shield-str Hot Reload Server Started\n";
    std::cout << "Listen on: http://localhost:8080\n";
    std::cout << "To update rules, send a POST request:\n";
    std::cout << "  curl -X POST http://localhost:8080/api/rules -d @config/rules.json\n";
    std::cout << "========================================\n";

    svr.listen("0.0.0.0", 8080);

    log_thread.join();
    return 0;
}
