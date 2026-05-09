# 📋 상세 개발계획서

**프로젝트명:** `shield-str` — High-Performance C++ Log Masking Utility
**문서 버전:** 1.0.0
**작성일:** 2026-05-09
**참조 문서:** `.ai/agent_log/docs/prd_shield-str.md` v1.1.0
**개발 방법론:** TDD (Test-Driven Development)

---

## 1. 개발 환경 및 기술 스택

### 1.1 언어 및 표준

| 항목 | 내용 |
|---|---|
| 언어 | C++17 |
| 빌드 시스템 | CMake 3.16 이상 |
| 패키지 관리 | CMake FetchContent |
| 컴파일러 | GCC 9+ / Clang 10+ |

### 1.2 핵심 의존성

| 라이브러리 | 용도 | 버전 |
|---|---|---|
| **Google RE2** | 선형 시간 정규식 엔진 | 2024-03-01 이상 |
| **simdjson** | 초고속 JSON 파서 (룰셋 로딩) | 3.x |
| **Google Test** | 단위 테스트 프레임워크 | 1.14.x |
| **Google Benchmark** | 성능 벤치마크 프레임워크 | 1.8.x |

### 1.3 개발 도구

| 도구 | 용도 |
|---|---|
| Valgrind | 메모리 누수 검증 |
| ThreadSanitizer (TSan) | Data Race 탐지 |
| AddressSanitizer (ASan) | 메모리 오류 탐지 |
| perf / Instruments | CPU 프로파일링 |
| Doxygen | API 문서 자동 생성 |
| GitHub Actions | CI/CD 파이프라인 |

---

## 2. 프로젝트 디렉토리 구조

```
shield-str/
├── CMakeLists.txt                  # 루트 빌드 정의
├── cmake/
│   └── FetchDependencies.cmake     # 외부 의존성 FetchContent 관리
├── include/
│   └── shield/
│       ├── shield.hpp              # 통합 헤더 (All-in-one include)
│       ├── MaskingEngine.hpp
│       ├── RuleManager.hpp
│       ├── RuleSet.hpp
│       ├── AhoCorasickScanner.hpp
│       └── Re2PatternMatcher.hpp
├── src/
│   ├── MaskingEngine.cpp
│   ├── RuleManager.cpp
│   ├── RuleSet.cpp
│   ├── AhoCorasickScanner.cpp
│   └── Re2PatternMatcher.cpp
├── tests/
│   ├── CMakeLists.txt
│   ├── unit/
│   │   ├── test_aho_corasick.cpp
│   │   ├── test_re2_matcher.cpp
│   │   ├── test_rule_manager.cpp
│   │   ├── test_masking_engine.cpp
│   │   └── test_thread_safety.cpp
│   └── bench/
│       ├── bench_no_mask.cpp       # BM-01: 마스킹 대상 없음
│       ├── bench_with_mask.cpp     # BM-02: 마스킹 대상 있음
│       └── bench_parallel.cpp      # BM-04: 병렬 처리
├── examples/
│   └── spdlog_sink/
│       ├── CMakeLists.txt
│       ├── ShieldSink.hpp
│       └── main.cpp
├── rules/
│   └── default_rules.json          # 기본 내장 룰셋 5종
├── .github/
│   └── workflows/
│       └── ci.yml                  # CI 파이프라인
├── .ai/
│   └── agent_log/
│       ├── docs/                   # 프로젝트 문서
│       ├── memory/                 # 장기 컨텍스트
│       └── tasks/                  # 작업 기록
└── .gitignore
```

---

## 3. 개발 단계별 상세 계획

### Phase 1: Core Engine & Fast-Path
**목표:** 기본 마스킹 파이프라인 동작 확인 및 성능 Baseline 측정

---

#### Task 1-1: 프로젝트 초기화

**작업 내용:**
- CMakeLists.txt 작성 (FetchContent: RE2, simdjson, GTest, GBenchmark)
- `.gitignore` 설정 (`build/`, `*.DS_Store` 등)
- GitHub Actions CI 기본 워크플로우 작성 (빌드 + 테스트)

**산출물:**
- `CMakeLists.txt`
- `cmake/FetchDependencies.cmake`
- `.github/workflows/ci.yml`
- `.gitignore`

**완료 조건:** `cmake -B build && cmake --build build` 성공

---

#### Task 1-2: AhoCorasickScanner 구현 (TDD)

**TC 먼저 작성 → Failed 확인 → 구현 → Passed 확인**

| TC ID | 테스트 케이스 | 기대 결과 |
|---|---|---|
| UT-01 | `build({"password"})` → `scan("user=admin password=1234")` | `true` 반환 |
| UT-02 | `build({"password","token"})` → `scan("token=abc")` | `true` 반환 |
| UT-03 | `build({"password"})` → `scan("user=admin id=1234")` | `false` 반환 |
| UT-04 | 빈 키워드 셋 → `scan("anything")` | `false` 반환 |
| UT-05 | 대소문자 구분 확인 → `scan("PASSWORD=1234")` | 설정에 따라 `false` 또는 `true` |

**구현 인터페이스:**
```cpp
class AhoCorasickScanner {
public:
    void build(const std::vector<std::string>& keywords);
    bool scan(std::string_view text) const;
private:
    // Aho-Corasick 트리 (goto, fail, output 함수)
};
```

**산출물:** `include/shield/AhoCorasickScanner.hpp`, `src/AhoCorasickScanner.cpp`, `tests/unit/test_aho_corasick.cpp`

---

#### Task 1-3: Re2PatternMatcher 구현 (TDD)

| TC ID | 테스트 케이스 | 기대 결과 |
|---|---|---|
| UT-06 | `"password=secret123"` → password 룰 적용 | `"password=***"` |
| UT-07 | `"Authorization: Bearer eyJhbG..."` → bearer 룰 적용 | `"Authorization: Bearer ***REDACTED***"` |
| UT-08 | `"email: user@example.com"` → email 룰 적용 | `"email: ***@***.***"` |
| UT-09 | 마스킹 대상 없는 문자열 | 원본 문자열 그대로 반환 |
| UT-10 | 복수 패턴 동시 존재 | 모든 패턴 각각 마스킹 적용 |

**구현 인터페이스:**
```cpp
class Re2PatternMatcher {
public:
    struct Rule {
        std::string id;
        std::unique_ptr<re2::RE2> pattern;
        int mask_group;         // 마스킹할 캡처 그룹 번호 (0=전체)
        std::string replacement;
    };

    void add_rule(Rule rule);
    // 매칭 결과: 마스킹 발생 시 새 string, 없으면 nullopt
    std::optional<std::string> apply(std::string_view text) const;
};
```

**산출물:** `include/shield/Re2PatternMatcher.hpp`, `src/Re2PatternMatcher.cpp`, `tests/unit/test_re2_matcher.cpp`

---

#### Task 1-4: MaskingEngine 기본 파이프라인 구현 (TDD)

| TC ID | 테스트 케이스 | 기대 결과 |
|---|---|---|
| UT-11 | 마스킹 대상 없음 → `process()` 반환 타입 | `std::string_view` (Zero-Copy 확인) |
| UT-12 | 마스킹 대상 있음 → `process()` 반환 타입 | `std::string` (새 버퍼) |
| UT-13 | 빈 문자열 입력 | 빈 `string_view` 반환 |
| UT-14 | JSON 포맷 로그 입력 | JSON Key 값 마스킹 정확도 확인 |

**구현 인터페이스:**
```cpp
class MaskingEngine {
public:
    explicit MaskingEngine(std::shared_ptr<RuleManager> rule_manager);

    using Result = std::variant<std::string_view, std::string>;
    Result process(std::string_view input) const;
};
```

**산출물:** `include/shield/MaskingEngine.hpp`, `src/MaskingEngine.cpp`, `tests/unit/test_masking_engine.cpp`

---

#### Task 1-5: 성능 Baseline 측정

**벤치마크 구현 및 목표 지표 달성 확인:**

| BM ID | 시나리오 | 목표 |
|---|---|---|
| BM-01 | 10KB 문자열, 마스킹 대상 없음 | p99 < **100ns** |
| BM-02 | 10KB 문자열, 마스킹 대상 1건 | p99 < **1ms** |

> 목표 미달 시 병목 구간 프로파일링 후 최적화 수행

**산출물:** `tests/bench/bench_no_mask.cpp`, `tests/bench/bench_with_mask.cpp`

**Phase 1 완료 조건:**
- [ ] 모든 단위 테스트 (UT-01 ~ UT-14) 100% Pass
- [ ] BM-01, BM-02 목표 지표 달성
- [ ] CI 파이프라인 Green

---

### Phase 2: Dynamic Rules & Thread Safety
**목표:** 무중단 룰 업데이트 및 멀티스레드 안전성 확보

---

#### Task 2-1: RuleSet (Immutable) 구현

**설계 원칙:** 한번 생성된 `RuleSet`은 불변(Immutable). `AhoCorasickScanner`와 `Re2PatternMatcher`를 소유.

```cpp
class RuleSet {
public:
    explicit RuleSet(std::vector<RuleDefinition> rules);

    const AhoCorasickScanner& scanner() const;
    const Re2PatternMatcher& matcher() const;

private:
    AhoCorasickScanner scanner_;
    Re2PatternMatcher matcher_;
};
```

**산출물:** `include/shield/RuleSet.hpp`, `src/RuleSet.cpp`

---

#### Task 2-2: RuleManager 구현 (TDD)

| TC ID | 테스트 케이스 | 기대 결과 |
|---|---|---|
| UT-15 | 정상 JSON 파일 로드 → `get_current_ruleset()` | 룰셋 반환 (nullptr 아님) |
| UT-16 | JSON 필드 누락 파일 로드 | 예외 또는 오류 코드 반환 |
| UT-17 | 잘못된 정규식 포함 JSON 로드 | 해당 룰 스킵 + 경고 로그 |
| UT-18 | `update_rules()` 호출 후 `get_current_ruleset()` | 새 룰셋 반환 확인 |

**구현 인터페이스:**
```cpp
class RuleManager {
public:
    static std::shared_ptr<RuleManager> from_file(std::string_view path);
    static std::shared_ptr<RuleManager> from_json(std::string_view json_str);

    void update_rules(std::string_view json_config);
    std::shared_ptr<const RuleSet> get_current_ruleset() const;

private:
    std::atomic<std::shared_ptr<const RuleSet>> current_ruleset_;
};
```

**산출물:** `include/shield/RuleManager.hpp`, `src/RuleManager.cpp`, `tests/unit/test_rule_manager.cpp`

---

#### Task 2-3: RCU 패턴 — Lock-Free 룰셋 스왑 구현 (TDD)

**핵심 구현:**
```cpp
// 룰 업데이트 (쓰기 측)
void RuleManager::update_rules(std::string_view json_config) {
    auto new_ruleset = parse_and_compile(json_config); // 새 객체 생성
    current_ruleset_.store(new_ruleset, std::memory_order_release);
    // 기존 shared_ptr은 마지막 참조자가 해제 시 자동 소멸
}

// 룰 조회 (읽기 측 — 로깅 스레드)
std::shared_ptr<const RuleSet> RuleManager::get_current_ruleset() const {
    return current_ruleset_.load(std::memory_order_acquire);
}
```

| TC ID | 테스트 케이스 | 기대 결과 |
|---|---|---|
| UT-19 | 64스레드 동시 `process()` 호출 중 `update_rules()` 실행 | Data Race 없음 (TSan 통과) |
| UT-20 | 룰 업데이트 후 즉시 새 룰셋으로 마스킹 수행 | 새 룰 적용된 결과 반환 |
| UT-21 | 업데이트 중 이전 룰셋 참조 중인 스레드 | 안전하게 이전 룰셋 사용 완료 후 해제 |

**산출물:** `tests/unit/test_thread_safety.cpp`

---

#### Task 2-4: 기본 내장 룰셋 5종 완성

**`rules/default_rules.json` 작성:**

| 룰 ID | 대상 | 정규식 |
|---|---|---|
| `rule_password` | Password/passwd 필드 | `(?i)(password\|passwd\|pwd)["']?\s*[:=]\s*["']?([^\s"',}]+)` |
| `rule_bearer_token` | HTTP Bearer 토큰 | `Bearer\s+([A-Za-z0-9\-._~+/]+=*)` |
| `rule_email` | 이메일 주소 | `[a-zA-Z0-9._%+\-]+@[a-zA-Z0-9.\-]+\.[a-zA-Z]{2,}` |
| `rule_phone_kr` | 한국 전화번호 | `01[016789]-?\d{3,4}-?\d{4}` |
| `rule_credit_card` | 신용카드 번호 | `\b\d{4}[- ]?\d{4}[- ]?\d{4}[- ]?\d{4}\b` |

**추가 TC:**

| TC ID | 테스트 케이스 |
|---|---|
| UT-22 | `default_rules.json` 로드 → 5개 룰 모두 파싱 성공 |
| UT-23 | 전화번호 패턴: `"010-1234-5678"` → `"***-****-****"` |
| UT-24 | 신용카드 패턴: `"4111-1111-1111-1111"` → `"****-****-****-****"` |

**Phase 2 완료 조건:**
- [ ] 모든 단위 테스트 (UT-15 ~ UT-24) 100% Pass
- [ ] TSan 빌드에서 Data Race 0건
- [ ] 기본 룰셋 5종 TC 모두 Pass

---

### Phase 3: Integration & Optimization
**목표:** 실사용 환경 검증, 성능 최적화, 배포 준비

---

#### Task 3-1: spdlog Custom Sink 구현 (FR-05)

**설계:**
```cpp
// examples/spdlog_sink/ShieldSink.hpp
template<typename Mutex>
class ShieldSink : public spdlog::sinks::base_sink<Mutex> {
public:
    explicit ShieldSink(
        std::shared_ptr<shield::MaskingEngine> engine,
        spdlog::sink_ptr downstream_sink
    );

protected:
    void sink_it_(const spdlog::details::log_msg& msg) override;
    void flush_() override;

private:
    std::shared_ptr<shield::MaskingEngine> engine_;
    spdlog::sink_ptr downstream_;
};
```

**산출물:** `examples/spdlog_sink/ShieldSink.hpp`, `examples/spdlog_sink/main.cpp`

---

#### Task 3-2: 병렬 처리 벤치마크 및 최적화

| BM ID | 시나리오 | 목표 |
|---|---|---|
| BM-03 | 1KB 문자열, 마스킹 5건 | p99 < 500μs |
| BM-04 | 64스레드 동시 `process()` | Throughput 측정 (선형 확장성 확인) |

**최적화 체크리스트:**
- [ ] 불필요한 `std::string` 복사 제거
- [ ] `RE2` 패턴 객체 스레드당 재사용 (`thread_local` 캐싱 검토)
- [ ] Aho-Corasick 트리 메모리 레이아웃 캐시 친화적 구조 확인

---

#### Task 3-3: 메모리 및 안전성 검증

**검증 항목:**

| 검증 도구 | 검증 대상 | 합격 기준 |
|---|---|---|
| **Valgrind** (memcheck) | 전체 단위 테스트 실행 | 메모리 누수 0건 |
| **ASan** (AddressSanitizer) | 전체 단위 테스트 실행 | 오류 0건 |
| **TSan** (ThreadSanitizer) | 멀티스레드 TC (UT-19~21) | Data Race 0건 |

**실행 방법 (CMake Preset):**
```bash
# ASan + UBSan 빌드
cmake -B build-asan -DCMAKE_CXX_FLAGS="-fsanitize=address,undefined" .
cmake --build build-asan && ctest --test-dir build-asan

# TSan 빌드
cmake -B build-tsan -DCMAKE_CXX_FLAGS="-fsanitize=thread" .
cmake --build build-tsan && ctest --test-dir build-tsan
```

---

#### Task 3-4: 문서화 및 릴리스 준비

**산출물:**

| 문서 | 내용 |
|---|---|
| `README.md` | 프로젝트 소개, 빌드 방법, 빠른 시작 예제 |
| `INSTALL.md` | FetchContent / 직접 빌드 설치 가이드 |
| `CHANGELOG.md` | v1.0.0 변경 이력 |
| Doxygen | Public API 자동 문서 생성 |

**릴리스 체크리스트:**
- [ ] 전체 TC (UT-01 ~ UT-24) 100% Pass
- [ ] 모든 성능 목표 (BM-01 ~ BM-04) 달성
- [ ] Valgrind / ASan / TSan 0건
- [ ] README.md 및 설치 가이드 완성
- [ ] GitHub Release 태그 `v1.0.0` 생성

---

## 4. TC 전체 목록 (Master Test List)

| TC ID | 분류 | 대상 모듈 | 테스트 내용 | Phase |
|---|---|---|---|---|
| UT-01 | Unit | AhoCorasickScanner | 단일 키워드 탐지 | 1 |
| UT-02 | Unit | AhoCorasickScanner | 다중 키워드 동시 탐지 | 1 |
| UT-03 | Unit | AhoCorasickScanner | 키워드 없음 → false | 1 |
| UT-04 | Unit | AhoCorasickScanner | 빈 키워드 셋 | 1 |
| UT-05 | Unit | AhoCorasickScanner | 대소문자 구분 동작 | 1 |
| UT-06 | Unit | Re2PatternMatcher | Password 마스킹 | 1 |
| UT-07 | Unit | Re2PatternMatcher | Bearer Token 마스킹 | 1 |
| UT-08 | Unit | Re2PatternMatcher | 이메일 마스킹 | 1 |
| UT-09 | Unit | Re2PatternMatcher | 대상 없음 → nullopt | 1 |
| UT-10 | Unit | Re2PatternMatcher | 복수 패턴 동시 처리 | 1 |
| UT-11 | Unit | MaskingEngine | 대상 없음 → string_view 반환 | 1 |
| UT-12 | Unit | MaskingEngine | 대상 있음 → string 반환 | 1 |
| UT-13 | Unit | MaskingEngine | 빈 문자열 입력 | 1 |
| UT-14 | Unit | MaskingEngine | JSON 포맷 로그 마스킹 | 1 |
| UT-15 | Unit | RuleManager | 정상 JSON 파일 로드 | 2 |
| UT-16 | Unit | RuleManager | 필드 누락 JSON → 오류 | 2 |
| UT-17 | Unit | RuleManager | 잘못된 정규식 → 스킵 + 경고 | 2 |
| UT-18 | Unit | RuleManager | update_rules 후 새 룰셋 반영 | 2 |
| UT-19 | Thread | RuleManager (RCU) | 64스레드 동시 처리 중 업데이트 | 2 |
| UT-20 | Thread | RuleManager (RCU) | 업데이트 즉시 새 룰 적용 | 2 |
| UT-21 | Thread | RuleManager (RCU) | 이전 룰셋 안전 해제 | 2 |
| UT-22 | Unit | RuleManager | default_rules.json 5종 파싱 | 2 |
| UT-23 | Unit | Re2PatternMatcher | 한국 전화번호 마스킹 | 2 |
| UT-24 | Unit | Re2PatternMatcher | 신용카드 번호 마스킹 | 2 |
| BM-01 | Bench | MaskingEngine | 10KB, 대상 없음 < 100ns | 1 |
| BM-02 | Bench | MaskingEngine | 10KB, 대상 1건 < 1ms | 1 |
| BM-03 | Bench | MaskingEngine | 1KB, 대상 5건 < 500μs | 3 |
| BM-04 | Bench | MaskingEngine | 64스레드 Throughput | 3 |

---

## 5. CI/CD 파이프라인

```yaml
# .github/workflows/ci.yml (요약)
name: CI

on: [push, pull_request]

jobs:
  build-and-test:
    runs-on: ubuntu-22.04
    steps:
      - uses: actions/checkout@v4
      - name: Configure
        run: cmake -B build -DCMAKE_BUILD_TYPE=Debug
      - name: Build
        run: cmake --build build -j4
      - name: Unit Tests
        run: ctest --test-dir build --output-on-failure

  sanitizer-check:
    runs-on: ubuntu-22.04
    strategy:
      matrix:
        sanitizer: [address, thread]
    steps:
      - uses: actions/checkout@v4
      - name: Configure with ${{ matrix.sanitizer }}san
        run: cmake -B build -DCMAKE_CXX_FLAGS="-fsanitize=${{ matrix.sanitizer }}"
      - name: Build & Test
        run: cmake --build build && ctest --test-dir build
```

---

## 6. 일정 계획 (Schedule)

| Phase | 주요 Task | 예상 소요 |
|---|---|---|
| **Phase 1** | 프로젝트 초기화, AhoCorasick, RE2Matcher, MaskingEngine, Benchmark | 2주 |
| **Phase 2** | RuleSet, RuleManager, RCU 패턴, 기본 룰셋 5종 | 2주 |
| **Phase 3** | spdlog 통합, 최적화, 검증, 문서화, 릴리스 | 1주 |
| **합계** | | **5주** |

---

## 7. 개발 규칙 및 컨벤션

### 7.1 코드 컨벤션

| 항목 | 규칙 |
|---|---|
| 네임스페이스 | 모든 공개 심볼은 `shield::` 네임스페이스 |
| 헤더 가드 | `#pragma once` 사용 |
| 인코딩 | UTF-8 |
| 인덴트 | 공백 4칸 |
| 명명 규칙 | `snake_case` (변수, 함수), `PascalCase` (클래스) |

### 7.2 TDD 사이클 규칙

```
1. TC 파일 작성 (tests/unit/test_*.cpp)
2. 빌드 → TC Failed 확인
3. 최소한의 구현으로 TC Pass
4. 리팩토링 (TC 유지)
5. 반복
```

### 7.3 커밋 규칙

- TC 추가: `test: UT-XX AhoCorasickScanner 단일 키워드 탐지 TC 추가`
- 구현 추가: `feat: AhoCorasickScanner 기본 키워드 탐지 구현`
- 버그 수정: `fix: Re2PatternMatcher 복수 패턴 누락 수정`
- 문서: `docs: RuleManager API 주석 추가`
