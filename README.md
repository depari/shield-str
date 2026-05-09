# shield-str

> **High-Performance C++ Log Masking Utility**  
> Zero-Overhead · Zero-Downtime Rule Update · Drop-in Integration

---

## 특징

- ⚡ **초고속:** 10KB 로그(마스킹 불필요) 처리 < 100ns (Zero-Copy, Zero-Allocation)
- 🔒 **안전:** Google RE2로 Catastrophic Backtracking 원천 차단
- 🔄 **무중단:** Lock-Free RCU 패턴으로 런타임 룰 교체 (재시작 불필요)
- 🧵 **Thread-Safe:** `process()`는 잠금 없이 모든 스레드에서 안전 호출 가능
- 📦 **쉬운 통합:** CMake FetchContent 3줄로 추가

---

## Quick Start (5분)

### 1. CMakeLists.txt

```cmake
include(FetchContent)
FetchContent_Declare(shield-str
    GIT_REPOSITORY https://github.com/your-org/shield-str.git
    GIT_TAG        v1.0.0
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(shield-str)
target_link_libraries(my_app PRIVATE shield::shield-str)
```

### 2. 코드

```cpp
#include <shield/shield.hpp>
#include <variant>
#include <iostream>

int main() {
    auto mgr    = shield::RuleManager::from_file("rules/default_rules.json");
    auto engine = std::make_shared<shield::MaskingEngine>(mgr);

    auto result = engine->process("user=alice password=S3cr3t!");

    if (std::holds_alternative<std::string>(result))
        std::cout << std::get<std::string>(result) << "\n";
    // 출력: user=alice password=***
}
```

---

## 요구사항

| 항목 | 최소 버전 |
|---|---|
| C++ | C++17 |
| CMake | 3.16+ |
| 컴파일러 | GCC 9+ / Clang 10+ |
| OS | Linux (Ubuntu 20.04+), macOS (12+) |

---

## 내장 의존성

| 라이브러리 | 용도 |
|---|---|
| Google RE2 | 선형 시간 정규식 엔진 |
| simdjson | 초고속 JSON 룰셋 파서 |

*FetchContent로 자동 다운로드됩니다.*

---

## 문서

| 문서 | 설명 |
|---|---|
| [Quick Start](docs/quick-start.md) | 30분 안에 첫 마스킹 결과 확인 |
| [API Reference](docs/api-reference.md) | 전체 Public API 상세 |
| [Integration Guide](docs/integration-guide.md) | spdlog/glog/커스텀 연동 |
| [Ruleset Guide](docs/ruleset-guide.md) | 커스텀 룰 JSON 작성 |

---

## 빌드 및 테스트

```bash
# 기본 빌드 + 테스트
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel 4
ctest --test-dir build --output-on-failure

# ASan 검증
cmake -B build-asan -DCMAKE_CXX_FLAGS="-fsanitize=address,undefined"
cmake --build build-asan && ctest --test-dir build-asan

# TSan 검증
cmake -B build-tsan -DCMAKE_CXX_FLAGS="-fsanitize=thread"
cmake --build build-tsan && ctest --test-dir build-tsan
```

---

## 라이선스

MIT License — [LICENSE](LICENSE)
