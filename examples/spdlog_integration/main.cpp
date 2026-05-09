#include "shield/RuleManager.hpp"
#include "shield/MaskingEngine.hpp"
#include "shield/integration/SpdlogMaskingSink.hpp"

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include <iostream>
#include <memory>

int main() {
    // 1. 룰 매니저 및 마스킹 엔진 초기화
    // 간단한 패스워드 마스킹 룰 설정
    const std::string rules_json = R"json({
      "version": "1.0",
      "rules": [
        {
          "id": "rule_password",
          "trigger_keywords": ["password"],
          "pattern": "(?i)password=(\\S+)",
          "mask_group": 1,
          "replacement": "***"
        }
      ]
    })json";
    
    auto mgr = shield::RuleManager::from_json(rules_json);
    auto engine = std::make_shared<shield::MaskingEngine>(mgr);

    // 2. spdlog 원본 콘솔 싱크 생성
    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    
    // 3. shield-str 래퍼 싱크로 감싸기
    auto masking_sink = std::make_shared<shield::integration::SpdlogMaskingSink_mt>(engine, console_sink);

    // 4. Logger 생성 및 기본 로거로 등록
    auto logger = std::make_shared<spdlog::logger>("shield_logger", masking_sink);
    logger->set_level(spdlog::level::info);
    spdlog::set_default_logger(logger);

    // 5. 로깅 테스트 (애플리케이션 코드는 로거만 사용, 내부에서 자동 마스킹 됨)
    spdlog::info("Application started successfully.");
    spdlog::info("User 'alice' login attempt with password=secret_pw123!");
    spdlog::warn("Invalid access token password=hidden_token_99");
    spdlog::error("Database connection failed.");

    return 0;
}
