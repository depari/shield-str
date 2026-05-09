# 🔌 Integration Guide — shield-str

기존 로깅 프레임워크에 shield-str을 연동하는 방법을 설명합니다.

---

## 1. spdlog 통합 (Custom Sink)

### 1.1 ShieldSink 헤더 복사

`examples/spdlog_sink/ShieldSink.hpp`를 프로젝트에 복사합니다.

### 1.2 spdlog 의존성 추가

```cmake
# CMakeLists.txt
FetchContent_Declare(spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG        v1.13.0
)
FetchContent_MakeAvailable(spdlog)
target_link_libraries(my_app PRIVATE shield::shield-str spdlog::spdlog)
```

### 1.3 코드 연동

```cpp
#include <shield/shield.hpp>
#include "ShieldSink.hpp"
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/logger.h>

int main() {
    // 1. shield-str 엔진 초기화
    auto mgr    = shield::RuleManager::from_file("rules/default_rules.json");
    auto engine = std::make_shared<shield::MaskingEngine>(mgr);

    // 2. 실제 출력 sink (파일, 콘솔 등)
    auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("app.log");

    // 3. ShieldSink 래핑 — 로그 → 마스킹 → 파일
    auto shield_sink = std::make_shared<shield::ShieldSink_mt>(engine, file_sink);

    // 4. Logger 생성
    auto logger = std::make_shared<spdlog::logger>("secure", shield_sink);
    logger->set_level(spdlog::level::debug);

    // 5. 사용 (민감 정보 자동 마스킹)
    logger->info("user={} password={}", "alice", "S3cr3t!");
    // app.log 에 기록: "user=alice password=***"

    return 0;
}
```

---

## 2. 직접 파이프라인 구성

spdlog 없이 자체 로깅 시스템에 통합하는 방법:

```cpp
#include <shield/shield.hpp>
#include <variant>

class MyLogger {
public:
    MyLogger(std::shared_ptr<shield::RuleManager> mgr)
        : engine_(std::make_shared<shield::MaskingEngine>(mgr)) {}

    void log(std::string_view message) {
        auto result = engine_->process(message);

        std::string_view safe_msg;
        std::string      buf;

        if (std::holds_alternative<std::string_view>(result)) {
            safe_msg = std::get<std::string_view>(result);
        } else {
            buf      = std::get<std::string>(result);
            safe_msg = buf;
        }

        // 실제 출력 (파일, 네트워크 등)
        write_to_sink(safe_msg);
    }

    // 런타임 룰 업데이트 (Lock-Free)
    void reload_rules(std::string_view new_json) {
        engine_->get_rule_manager()->update_rules(new_json);
    }

private:
    std::shared_ptr<shield::MaskingEngine> engine_;
    void write_to_sink(std::string_view msg) { /* ... */ }
};
```

---

## 3. 멀티스레드 환경에서 안전한 사용

`MaskingEngine::process()`는 **완전 Thread-Safe**입니다.  
여러 스레드에서 동시에 같은 `engine` 인스턴스를 호출해도 안전합니다.

```cpp
// 공유 엔진 인스턴스 — 모든 스레드에서 공유
auto engine = std::make_shared<shield::MaskingEngine>(mgr);

// 각 스레드에서 동시 호출 가능
std::thread t1([&]() { engine->process("log1 password=abc"); });
std::thread t2([&]() { engine->process("log2 token=xyz"); });
t1.join(); t2.join();
```

---

## 4. 룰 핫 업데이트

```cpp
// 파일 감시 스레드 또는 시그널 핸들러에서
void on_config_change(const std::string& new_config_json) {
    try {
        mgr->update_rules(new_config_json);  // Lock-Free, 즉시 반영
        std::cout << "Rules updated successfully\n";
    } catch (const shield::RuleLoadError& e) {
        // 실패 시 기존 룰셋 유지 — 서비스 중단 없음
        std::cerr << "Rule update failed (keeping old rules): " << e.what() << "\n";
    }
}
```
