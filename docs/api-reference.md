# 📖 API Reference — shield-str

모든 Public API는 `shield::` 네임스페이스에 포함됩니다.  
단일 헤더: `#include <shield/shield.hpp>`

---

## MaskingEngine

```cpp
class shield::MaskingEngine
```

로그 마스킹 파이프라인의 진입점.

### Result 타입

```cpp
using Result = std::variant<std::string_view, std::string>;
```

| 타입 | 의미 |
|---|---|
| `std::string_view` | 입력 원본 참조 (Zero-Copy, 민감 정보 없음) |
| `std::string` | 마스킹 완료된 새 문자열 |

### 생성자

```cpp
explicit MaskingEngine(std::shared_ptr<RuleManager> rule_manager);
```

- `rule_manager`가 `nullptr`이면 `std::invalid_argument` 예외 발생.

### process()

```cpp
[[nodiscard]] Result process(std::string_view input) const;
```

| 파라미터 | 설명 |
|---|---|
| `input` | 검사할 로그 문자열. `string_view`가 반환될 경우 `input`의 수명이 `Result`보다 길어야 함. |

**처리 흐름:**
1. `input` 빈 문자열 → `string_view` 즉시 반환
2. Aho-Corasick으로 트리거 키워드 탐색 → 없으면 `string_view` 즉시 반환
3. RE2 패턴 매칭 → 매칭 없으면 `string_view` 반환, 있으면 마스킹된 `string` 반환

---

## RuleManager

```cpp
class shield::RuleManager
```

룰 생명주기 및 Lock-Free 교체 관리.

### from_file()

```cpp
static std::shared_ptr<RuleManager> from_file(std::string_view path);
```

JSON 파일 경로에서 룰을 로드합니다.  
**예외:** `shield::RuleLoadError` (파일 열기 실패 또는 JSON 파싱 오류)

### from_json()

```cpp
static std::shared_ptr<RuleManager> from_json(std::string_view json_str);
```

JSON 문자열에서 룰을 로드합니다.  
**예외:** `shield::RuleLoadError`

### update_rules()

```cpp
void update_rules(std::string_view json_config);
```

런타임 룰 교체. **Lock-Free** (읽기 스레드 Blocking 없음).  
내부적으로 `std::atomic<std::shared_ptr<const RuleSet>>::store()`로 구현됨.  
**예외:** `shield::RuleLoadError` (파싱 실패 시 기존 룰셋 유지)

### get_current_ruleset()

```cpp
[[nodiscard]] std::shared_ptr<const RuleSet> get_current_ruleset() const;
```

현재 활성 룰셋을 반환. acquire 메모리 순서 보장.

---

## RuleLoadError

```cpp
class shield::RuleLoadError : public std::runtime_error
```

JSON 파싱 또는 룰 컴파일 실패 시 발생하는 예외.

---

## RuleSet (내부 — 직접 생성 불필요)

```cpp
class shield::RuleSet  // Immutable
```

| 메서드 | 설명 |
|---|---|
| `rule_count()` | 컴파일된 룰 수 반환 |
| `scanner()` | `AhoCorasickScanner` 참조 반환 |
| `matcher()` | `PatternMatcher` 참조 반환 |

---

## 에러 처리 예시

```cpp
try {
    auto mgr = shield::RuleManager::from_file("rules/custom.json");
} catch (const shield::RuleLoadError& e) {
    // 파일 없음, JSON 오류, 등
    std::cerr << "Rule load failed: " << e.what() << "\n";
    // fallback: 기본 룰셋 사용
    auto mgr = shield::RuleManager::from_file("rules/default_rules.json");
}
```

---

## 성능 특성

| 시나리오 | 반환 타입 | 메모리 할당 |
|---|---|---|
| 민감 정보 없음 | `string_view` | **없음** |
| 민감 정보 있음 | `string` | 마스킹 결과 크기만큼 1회 |
| 빈 입력 | `string_view` | **없음** |
