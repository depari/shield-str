# ⚡ Quick Start — shield-str

> **목표:** 이 문서만 읽고 **30분 안에** 자신의 C++ 프로젝트에 shield-str을 통합하여 첫 마스킹 결과를 확인한다.

---

## 요구사항

| 항목 | 최소 버전 |
|---|---|
| C++ 컴파일러 | GCC 9+ 또는 Clang 10+ (C++17 지원) |
| CMake | 3.16 이상 |
| 인터넷 연결 | FetchContent 의존성 자동 다운로드 |

---

## Step 1: CMakeLists.txt에 3줄 추가

기존 `CMakeLists.txt`에 아래 3줄을 추가합니다.

```cmake
include(FetchContent)
FetchContent_Declare(
    shield-str
    GIT_REPOSITORY https://github.com/your-org/shield-str.git
    GIT_TAG        v1.0.0
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(shield-str)

# 내 타겟에 링크
target_link_libraries(my_app PRIVATE shield::shield-str)
```

---

## Step 2: 코드 작성 (10줄)

```cpp
#include <shield/shield.hpp>
#include <iostream>
#include <variant>

int main() {
    // 1. 룰 로드
    auto mgr = shield::RuleManager::from_file("rules/default_rules.json");
    // 또는 JSON 문자열로:
    // auto mgr = shield::RuleManager::from_json(R"({...})");

    // 2. 엔진 생성
    auto engine = std::make_shared<shield::MaskingEngine>(mgr);

    // 3. 마스킹 처리
    auto result = engine->process("user=alice password=S3cr3t!");

    // 4. 결과 출력
    if (std::holds_alternative<std::string>(result)) {
        std::cout << std::get<std::string>(result) << "\n";
        // 출력: user=alice password=***
    } else {
        std::cout << std::get<std::string_view>(result) << "\n";
        // 민감 정보 없음 — Zero-Copy 반환
    }
}
```

---

## Step 3: 빌드 및 실행

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel 4
./build/my_app
```

**예상 출력:**
```
user=alice password=***
```

---

## Step 4: 기본 내장 룰셋 사용

`rules/default_rules.json`을 프로젝트에 복사한 후:

```cpp
auto mgr = shield::RuleManager::from_file("path/to/default_rules.json");
```

기본 내장 룰 5종:

| 룰 ID | 대상 | 예시 |
|---|---|---|
| `rule_password` | password/passwd/pwd 필드 | `password=***` |
| `rule_bearer_token` | HTTP Bearer 토큰 | `Bearer ***REDACTED***` |
| `rule_email` | 이메일 주소 | `***@***.***` |
| `rule_phone_kr` | 한국 전화번호 | `***-****-****` |
| `rule_credit_card` | 신용카드 번호 | `****-****-****-****` |

---

## Step 5: 런타임 룰 업데이트 (무중단)

```cpp
// 애플리케이션 재시작 없이 즉시 반영
mgr->update_rules(new_json_config);  // Lock-Free
```

---

## 다음 단계

- [API Reference](api-reference.md) — 전체 Public API 상세
- [Integration Guide](integration-guide.md) — spdlog/glog 연동
- [Ruleset Guide](ruleset-guide.md) — 커스텀 룰 작성
