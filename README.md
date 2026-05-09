# shield-str (High-Performance Log Masking Library)

`shield-str`는 C++ 기반 애플리케이션에서 민감 정보(PII, 인증 토큰, 비밀번호 등)를 실시간으로 안전하게 마스킹하기 위해 설계된 **초고속/저지연(Zero-copy)** 로깅 보안 라이브러리입니다.

특히 방대한 양의 로그가 쏟아지는 프로덕션 환경에서도 성능 병목 없이 동작할 수 있도록 최적화되어 있으며, 어플리케이션을 재시작하지 않고도 마스킹 룰을 동적으로 교체(Hot Reload)할 수 있습니다.

## ✨ 주요 기능 (Features)

* **초고속 처리 성능 (Zero-Copy Fallback):**
  - 마스킹 대상 키워드가 없는 99%의 일반 로그는 메모리 복사 및 할당 없이(Zero-Allocation) 즉시 통과(`std::string_view` 반환)합니다.
  - 구글 `RE2` 정규식 엔진 및 `simdjson`과 결합하여 타 라이브러리 대비 압도적인 마스킹 속도를 보장합니다.
* **표준 라이브러리(Stdlib) Only 모드 지원:**
  - 외부 의존성(RE2, simdjson 등) 반입이 제한적인 폐쇄망 환경을 위해, 100% C++ 표준 라이브러리(`std::regex`, 수동 파싱)만으로 구동 가능한 `SHIELD_USE_STD_ONLY` 옵션을 제공합니다.
* **부분 마스킹 (Format-Preserving Masking) 지원:**
  - `***-****-1234`와 같이 캡처 그룹 백레퍼런스(`$N`)를 활용하여 원본 데이터의 형태를 유지한 채 핵심 민감정보만 마스킹할 수 있습니다.
* **Lock-free 동적 룰 교체 (Hot Reload):**
  - RCU(Read-Copy-Update) 아키텍처 기반으로, 서버가 구동 중인 상태에서도 트래픽 중단이나 동시성 병목 없이 JSON 포맷의 마스킹 정책을 실시간 주입할 수 있습니다.
* **spdlog 완전 투명 연동 (Transparent Logging):**
  - 기존 비즈니스 로깅 코드 수정 0줄! 제공되는 `SpdlogMaskingSink` 래퍼 하나만 적용하면 자동으로 모든 `spdlog` 로그에 마스킹이 적용됩니다.

---

## 🚀 빠른 시작 (Quick Start)

### 1. CMake 연동 (FetchContent)

프로젝트의 `CMakeLists.txt`에 아래 내용을 추가하여 손쉽게 라이브러리를 포함할 수 있습니다.

```cmake
include(FetchContent)

FetchContent_Declare(
    shield-str
    GIT_REPOSITORY https://github.com/your-org/shield-str.git
    GIT_TAG        main
)
FetchContent_MakeAvailable(shield-str)

# (선택) 외부 라이브러리 없이 C++ 표준 모드로 빌드하려면:
# set(SHIELD_USE_STD_ONLY ON CACHE BOOL "Use stdlib only" FORCE)

target_link_libraries(my_app PRIVATE shield::shield-str)
```

### 2. 기본 사용법 (Basic C++ Usage)

```cpp
#include "shield/RuleManager.hpp"
#include "shield/MaskingEngine.hpp"
#include <iostream>

int main() {
    // 1. JSON 룰 로드 (파일 경로 또는 문자열 지원)
    auto mgr = shield::RuleManager::from_file("config/rules.json");
    shield::MaskingEngine engine(mgr);

    // 2. 로그 마스킹 처리
    std::string log = "User bob login attempt password=secret_token_123!";
    auto masked = engine.process(log);

    // 3. Variant 언래핑 후 사용 (Zero-copy 최적화)
    std::cout << "[Filtered] " << masked.value_or(log) << std::endl;
    return 0;
}
```

---

## 🛠 고급 활용 가이드 (Advanced Usage)

### [A] spdlog 투명 연동 (Zero-Code Modification)
기존에 `spdlog`를 사용 중이시라면 비즈니스 로직 수정 없이 `SpdlogMaskingSink` 래퍼를 통해 엔진을 결합할 수 있습니다.

```cpp
#include "shield/integration/SpdlogMaskingSink.hpp"
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>

// 원본 콘솔 싱크 생성
auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();

// shield-str 래퍼 싱크로 원본 감싸기
auto mgr = shield::RuleManager::from_file("config/rules.json");
auto engine = std::make_shared<shield::MaskingEngine>(mgr);
auto masking_sink = std::make_shared<shield::integration::SpdlogMaskingSink_mt>(engine, console_sink);

// 전역 로거 교체
auto logger = std::make_shared<spdlog::logger>("shield_logger", masking_sink);
spdlog::set_default_logger(logger);

// 이제 기존 로깅 코드를 1줄도 수정할 필요가 없습니다! 자동으로 마스킹됩니다.
spdlog::info("Database auth authtoken=X1Y2Z3A4B9 failed."); 
// 출력: [shield_logger] [info] Database auth authtoken=***AUTH*** failed.
```

### [B] JSON 룰셋 포맷 (Format-Preserving Masking)
`config/rules.json` 파일에 정규식 패턴과 캡처 그룹(Captured Groups) 기반 교체 포맷을 정의할 수 있습니다.

```json
{
  "version": "1.1",
  "rules": [
    {
      "id": "rule_phone",
      "description": "한국 전화번호 부분 마스킹",
      "trigger_keywords": ["010", "011"],
      "pattern": "(01[016789])-?(\\d{3,4})-?(\\d{4})",
      "mask_group": 0,
      "replacement": "$1-****-$3"
    },
    {
      "id": "rule_auth",
      "trigger_keywords": ["authtoken", "authcode"],
      "pattern": "(?i)(?:authtoken|authcode)[\"']?\\s*[:=]\\s*[\"']?([^\\s\"']+)",
      "mask_group": 1,
      "replacement": "***AUTH***"
    }
  ]
}
```
* `$1-****-$3`: 첫 번째와 세 번째 그룹 값은 보존하고 두 번째 그룹을 `****`로 부분 마스킹합니다.

### [C] 백그라운드 동적 룰셋 업데이트 (Hot Reload)
운영 중인 서버 프로세스를 재시작하지 않고도 관리자 API 등을 통해 새로운 룰을 실시간 주입할 수 있습니다.

```cpp
// ... 외부에서 JSON 문자열(new_rules_json) 수신 시
mgr->update_rules(new_rules_json);
// 백그라운드의 기존 스레드들은 병목(lock) 없이 매끄럽게 새 룰셋 기반으로 전환됩니다.
```
> 구체적인 HTTP 핫 리로드 서버 예제 코드는 `examples/hot_reload_server/main.cpp`를 참고하세요.

---

## 🔬 테스트 및 벤치마크 (Test & Benchmark)

`shield-str`은 TDD 방법론 하에 100% 테스트 커버리지를 지향합니다.

```bash
# 1. 의존성 다운로드 및 빌드 구성
cmake -B build -S . -DSHIELD_BUILD_TESTS=ON -DSHIELD_BUILD_BENCHMARKS=ON

# 2. 전체 컴파일
cmake --build build --parallel 4

# 3. 단위 테스트 수행
cd build && ctest --output-on-failure

# 4. 성능 벤치마크 수행
./tests/bench/bench_with_mask
```

## 📜 License
This project is licensed under the MIT License.
