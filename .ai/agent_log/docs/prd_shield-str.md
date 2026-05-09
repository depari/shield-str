# 📄 Product Requirements Document (PRD)

**프로젝트명:** `shield-str`
**부제:** High-Performance C++ Log Masking Utility
**문서 버전:** 1.2.0
**작성일:** 2026-05-09
**작성자:** AI Agent (기반 문서: `.human/prd.md`)
**상태:** Draft

---

## 1. 프로젝트 개요 (Overview)

`shield-str`은 C++ 애플리케이션의 로그 출력 파이프라인에 삽입되는 **초고속 민감 정보 마스킹 유틸리티 라이브러리**이다.

JSON, Plain Text 등 다양한 포맷의 로그 문자열에서 Password, Token, PII(개인식별정보) 등을 **런타임에 식별하고 마스킹(`***`)** 처리한다. 핵심 설계 원칙은 다음 두 가지이다.

- **Zero-Overhead:** 로깅 스레드에 성능 병목을 일으키지 않는다.
- **Zero-Downtime Rule Update:** 애플리케이션 재시작 없이 마스킹 룰을 교체한다.

---

## 2. 배경 및 목표 (Background & Objectives)

### 2.1 배경

시스템 모니터링과 디버깅을 위한 상세 로그는 필수적이나, 로그 수집 과정에서 사용자 PII 및 시스템 보안 토큰이 노출되는 보안 사고가 빈번히 발생한다. 기존 솔루션들은 다음 문제를 갖는다.

| 문제 | 설명 |
|---|---|
| 성능 저하 | `std::regex` 기반 처리의 Catastrophic Backtracking으로 CPU 스파이크 발생 |
| 불필요한 메모리 할당 | 매 호출마다 문자열 복사 및 힙 할당 발생 |
| 정적 룰 | 룰 변경 시 애플리케이션 재배포 필요 |

### 2.2 목표

1. 기존 C++ 로깅 프레임워크(spdlog, glog 등)에 **최소한의 코드 변경**으로 통합 가능한 독립 모듈 제공
2. 초당 수만 건의 로그가 발생하는 환경에서 **로깅 스레드 병목 없는 처리 속도** 달성
3. **애플리케이션 재시작 없이** 마스킹 룰 추가·수정 즉각 반영
4. CMake FetchContent / vcpkg / pkg-config 등 **표준 C++ 패키지 관리 방식으로 쉽게 프로젝트에 추가** 가능
5. Quick Start 가이드, API Reference, 통합 예제를 포함한 **완성된 문서 패키지** 제공으로 첫 사용까지 30분 내 완료

---

## 3. 사용자 및 사용 시나리오 (Users & Use Cases)

### 3.1 대상 사용자

| 사용자 유형 | 설명 |
|---|---|
| **C++ 백엔드 개발자** | 기존 로깅 파이프라인에 마스킹 레이어를 추가하려는 개발자 |
| **보안/컴플라이언스 엔지니어** | GDPR, PCI-DSS 등 규제 준수를 위해 로그 내 PII 차단이 필요한 역할 |
| **DevOps/SRE** | 런타임 룰 업데이트를 무중단으로 적용해야 하는 운영 담당자 |

### 3.2 핵심 사용 시나리오

**UC-01: 기본 마스킹 적용**
```
개발자가 MaskingEngine::process(log_string_view)를 호출
→ 마스킹 대상 키워드 탐색 (Aho-Corasick, O(N))
→ [대상 없음] 즉시 원본 string_view 반환 (Zero-Copy)
→ [대상 있음] RE2 정규식으로 정밀 매칭 후 마스킹된 string 반환
```

**UC-02: 무중단 룰 업데이트**
```
운영자가 rule.json 파일 수정 또는 API 호출
→ RuleManager가 변경 감지
→ 새 RuleSet 객체 생성 및 컴파일
→ atomic<shared_ptr<RuleSet>> swap (Lock-Free)
→ 진행 중인 로깅 스레드는 영향 없음
```

---

## 4. 기능 요구사항 (Functional Requirements)

| ID | 기능명 | 설명 | 우선순위 |
|---|---|---|---|
| **FR-01** | 문자열 마스킹 파이프라인 | 입력 로그 문자열에서 민감 패턴을 탐색하여 지정 문자(`*`)로 치환 반환 | P0 |
| **FR-02** | 다중 포맷 지원 | Plain Text 키워드 주변 값 식별 및 JSON Key 값 마스킹 모두 지원 | P0 |
| **FR-03** | 기본 내장 룰셋 | 이메일, 전화번호, Bearer Token, Password 등 기본 정규식 템플릿 내장 | P1 |
| **FR-04** | 동적 룰 로딩 | JSON/YAML 설정 파일로부터 마스킹 룰(키워드, 정규식, 치환 문자) 파싱 및 적용 | P1 |
| **FR-05** | spdlog 통합 예제 | Custom Sink 형태의 spdlog 연동 예제 코드 제공 | P2 |
| **FR-06** | 룰 유효성 검증 | 설정 파일 파싱 시 정규식 컴파일 오류, 필수 필드 누락 등을 사전 검증하고 오류 보고 | P1 |
| **FR-07** | CMake 패키지 설치 지원 | `find_package(shield-str)` 및 CMake FetchContent로 외부 프로젝트에서 바로 사용 가능 | P1 |
| **FR-08** | 단일 헤더 통합 | `#include <shield/shield.hpp>` 하나로 전체 API 접근 가능한 통합 헤더 제공 | P1 |
| **FR-09** | 가이드 문서 패키지 | Quick Start, API Reference, 통합 예제(spdlog/glog), 룰셋 작성 가이드로 구성된 문서 제공 | P1 |

---

## 5. 비기능 요구사항 (Non-Functional Requirements)

### 5.1 성능 (Performance) — 최우선

| ID | 요구사항 | 상세 |
|---|---|---|
| **NFR-01** | Zero-Copy 스캐닝 | 입력 문자열을 `std::string_view`로 전달, 불필요한 힙 할당 금지. 마스킹 발생 시에만 `reserve()` 후 버퍼 생성 |
| **NFR-02** | Fast-Path Early Exit | Aho-Corasick(`O(N)`)으로 트리거 키워드 선탐색. 키워드 미존재 시 정규식 엔진 호출 생략, 원본 `string_view` 즉시 반환 |
| **NFR-03** | 고성능 정규식 엔진 | `std::regex` 사용 금지. Google **RE2** 채택으로 선형 시간 보장, Catastrophic Backtracking 원천 차단 |

**목표 성능 지표:**

| 시나리오 | 목표 지연 시간 |
|---|---|
| 10KB 문자열, 마스킹 대상 없음 | **100ns 이하** |
| 10KB 문자열, 마스킹 대상 존재 | **1ms 이하** |

### 5.2 안전성 — 멀티스레드

| ID | 요구사항 | 상세 |
|---|---|---|
| **NFR-04** | Lock-Free 룰 업데이트 (RCU 패턴) | `std::atomic<std::shared_ptr<RuleSet>>`을 활용한 Read-Copy-Update 구현. 로깅 스레드 Blocking 없이 룰 객체 원자적 스왑 |

### 5.3 호환성 및 제약

| ID | 요구사항 | 상세 |
|---|---|---|
| **NFR-05** | C++ 표준 | **C++17** 이상 (`std::string_view`, structured bindings, `if constexpr` 활용) |
| **NFR-06** | 의존성 최소화 | 핵심 의존성: `RE2`(정규식), `simdjson`(룰셋 파싱) 2종으로 제한 |
| **NFR-07** | 빌드 시스템 | **CMake** 3.16 이상. FetchContent로 의존성 자동 관리 |
| **NFR-08** | 플랫폼 | Linux(Ubuntu 20.04+), macOS(12+) 지원. Windows는 Phase 3 이후 검토 |

### 5.4 외부 사용 편의성 — Developer Experience (DX)

| ID | 요구사항 | 상세 |
|---|---|---|
| **NFR-09** | 패키지 통합 | `CMakeLists.txt` 3줄로 FetchContent 추가 완료. `find_package` 지원으로 시스템 설치 후 바로 사용 가능 |
| **NFR-10** | 단순한 API Surface | 일반 사용 시 `MaskingEngine` 단일 클래스만으로 완결. 고급 기능은 선택적으로 접근 |
| **NFR-11** | Quick Start 달성 시간 | 문서를 처음 접한 개발자가 **30분 내** 자신의 프로젝트에 통합하여 첫 마스킹 결과 확인 가능 |
| **NFR-12** | 문서 완성도 | Quick Start / API Reference / 통합 예제 / 룰셋 작성 가이드 4종 문서 모두 v1.0.0 릴리스 시 포함 |

---

## 6. 시스템 아키텍처 (Architecture)

### 6.1 모듈 구성

```
shield-str/
├── include/shield/
│   ├── MaskingEngine.hpp      # 진입점 (Public API)
│   ├── RuleManager.hpp        # 룰 생명주기 관리
│   ├── RuleSet.hpp            # 불변(Immutable) 룰 컨테이너
│   ├── AhoCorasickScanner.hpp # 키워드 고속 탐색기
│   └── Re2PatternMatcher.hpp  # 정규식 매칭기
├── src/
│   └── (구현 파일)
├── tests/
│   ├── unit/                  # Google Test 단위 테스트
│   └── bench/                 # Google Benchmark 성능 테스트
├── examples/
│   └── spdlog_sink/           # spdlog 통합 예제
├── rules/
│   └── default_rules.json     # 기본 내장 룰셋
└── CMakeLists.txt
```

### 6.2 처리 파이프라인

```
입력: std::string_view
       │
       ▼
┌─────────────────────────────┐
│  MaskingEngine::process()   │
│                             │
│  1. atomic load → RuleSet   │
│  2. AhoCorasickScanner      │  ── 키워드 없음 → string_view 즉시 반환
│     (O(N) 키워드 탐색)       │
│  3. Re2PatternMatcher       │
│     (정밀 패턴 매칭)          │
│  4. 치환 문자열 빌드          │
└─────────────────────────────┘
       │
       ▼
출력: std::string (마스킹 완료) 또는 std::string_view (원본, 무변경)
```

### 6.3 모듈별 책임

| 모듈 | 책임 |
|---|---|
| `MaskingEngine` | Public API 진입점. `string_view` 수신 후 파이프라인 오케스트레이션 |
| `RuleManager` | `simdjson`으로 설정 파일 파싱. `RuleSet` 생명주기 및 atomic 교체 관리 |
| `RuleSet` (Immutable) | `AhoCorasickScanner`와 `Re2PatternMatcher` 소유. 읽기 전용 |
| `AhoCorasickScanner` | 모든 룰의 트리거 키워드를 Aho-Corasick 트리로 사전 컴파일. O(N) 탐색 |
| `Re2PatternMatcher` | 런타임 컴파일 비용 제거를 위해 `re2::RE2` 객체 사전 컴파일 및 보관 |

---

## 7. 룰셋 스펙 (Rule Set Specification)

### 7.1 JSON 룰셋 포맷 (예시)

```json
{
  "version": "1.0",
  "rules": [
    {
      "id": "rule_password",
      "description": "Password 필드 마스킹",
      "trigger_keywords": ["password", "passwd", "pwd"],
      "pattern": "(?i)(password|passwd|pwd)[\"']?\\s*[:=]\\s*[\"']?([^\\s\"',}]+)",
      "mask_group": 2,
      "replacement": "***"
    },
    {
      "id": "rule_bearer_token",
      "description": "Bearer Token 마스킹",
      "trigger_keywords": ["Bearer", "Authorization"],
      "pattern": "Bearer\\s+([A-Za-z0-9\\-._~+/]+=*)",
      "mask_group": 1,
      "replacement": "***REDACTED***"
    },
    {
      "id": "rule_email",
      "description": "이메일 주소 마스킹",
      "trigger_keywords": ["@"],
      "pattern": "[a-zA-Z0-9._%+\\-]+@[a-zA-Z0-9.\\-]+\\.[a-zA-Z]{2,}",
      "mask_group": 0,
      "replacement": "***@***.***"
    }
  ]
}
```

### 7.2 기본 내장 룰 목록

| ID | 대상 | 트리거 키워드 |
|---|---|---|
| `rule_password` | Password/passwd 필드 | `password`, `passwd`, `pwd` |
| `rule_bearer_token` | HTTP Bearer 토큰 | `Bearer`, `Authorization` |
| `rule_email` | 이메일 주소 | `@` |
| `rule_phone_kr` | 한국 전화번호 | `010`, `011`, `016`, `017`, `018`, `019` |
| `rule_credit_card` | 신용카드 번호 | `card`, `credit` |

---

## 8. 공개 API (Public API)

```cpp
namespace shield {

// 진입점: 싱글톤 또는 직접 인스턴스 생성
class MaskingEngine {
public:
    explicit MaskingEngine(std::shared_ptr<RuleManager> rule_manager);

    // 마스킹 대상 없을 시 입력 string_view 그대로 반환 (Zero-Copy)
    // 마스킹 발생 시 새 std::string 반환
    std::variant<std::string_view, std::string>
    process(std::string_view input) const;
};

class RuleManager {
public:
    // JSON 파일로부터 룰 로드
    static std::shared_ptr<RuleManager> from_file(std::string_view path);

    // 런타임 룰 교체 (Lock-Free, RCU 패턴)
    void update_rules(std::string_view json_config);

    std::shared_ptr<const RuleSet> get_current_ruleset() const;
};

} // namespace shield
```

---

## 9. 테스트 전략 (Test Strategy)

TDD 방법론을 적용한다. 구현 전 TC를 먼저 작성하고, TC failed → 구현 → TC pass 사이클을 반복한다.

### 9.1 단위 테스트 (Google Test)

| TC ID | 테스트 대상 | 시나리오 |
|---|---|---|
| UT-01 | `AhoCorasickScanner` | 단일 키워드 탐지 |
| UT-02 | `AhoCorasickScanner` | 다중 키워드 동시 탐지 |
| UT-03 | `AhoCorasickScanner` | 키워드 없는 문자열 → 즉시 반환 |
| UT-04 | `Re2PatternMatcher` | Password 패턴 마스킹 정확도 |
| UT-05 | `Re2PatternMatcher` | Bearer Token 마스킹 정확도 |
| UT-06 | `Re2PatternMatcher` | 이메일 마스킹 정확도 |
| UT-07 | `RuleManager` | JSON 룰셋 정상 파싱 |
| UT-08 | `RuleManager` | 잘못된 JSON → 예외/오류 반환 |
| UT-09 | `MaskingEngine` | 마스킹 대상 없음 → Zero-Copy 반환 확인 |
| UT-10 | `MaskingEngine` | 마스킹 대상 존재 → 올바른 치환 확인 |
| UT-11 | `RuleManager` (RCU) | 멀티스레드 환경 룰 업데이트 중 데이터 레이스 없음 확인 |

### 9.2 성능 벤치마크 (Google Benchmark)

| BM ID | 시나리오 | 목표 |
|---|---|---|
| BM-01 | 10KB 문자열, 마스킹 대상 없음 | p99 < 100ns |
| BM-02 | 10KB 문자열, 마스킹 대상 1건 | p99 < 1ms |
| BM-03 | 1KB 문자열, 마스킹 대상 다수(5건) | p99 < 500μs |
| BM-04 | 병렬 64스레드, 동시 `process()` 호출 | Throughput 측정 |

---

## 10. 마일스톤 및 릴리스 계획 (Milestones)

### Phase 1: Core Engine & Fast-Path
**목표:** 기본 마스킹 파이프라인 동작 확인

- [ ] CMake 프로젝트 초기화 (FetchContent: RE2, Google Test, Google Benchmark)
- [ ] `AhoCorasickScanner` 구현 및 단위 테스트 (UT-01 ~ UT-03)
- [ ] `Re2PatternMatcher` 구현 및 단위 테스트 (UT-04 ~ UT-06)
- [ ] `MaskingEngine` 기본 파이프라인 구현 (UT-09, UT-10)
- [ ] 성능 Baseline 측정 (BM-01, BM-02)

### Phase 2: Dynamic Rules & Thread Safety
**목표:** 무중단 룰 업데이트 및 멀티스레드 안전성 확보

- [ ] `simdjson` 연동 및 `RuleManager` JSON 파서 구현 (UT-07, UT-08)
- [ ] RCU 패턴(`std::atomic<shared_ptr>`) 룰셋 스왑 구현
- [ ] 멀티스레드 스트레스 테스트 (UT-11)
- [ ] 기본 내장 룰셋 5종 완성

### Phase 3: Integration, Documentation & Release
**목표:** 실사용 환경 검증, 가이드 문서 완성, 배포 준비

- [ ] spdlog Custom Sink 연동 예제 작성 (FR-05)
- [ ] CMake `find_package` / FetchContent 패키지 설치 지원 (FR-07)
- [ ] Valgrind 메모리 누수 검증
- [ ] perf / VTune CPU 프로파일링 및 병목 제거
- [ ] **가이드 문서 패키지 작성 (FR-09, NFR-12)**
  - `docs/quick-start.md` — 설치부터 첫 마스킹 결과까지
  - `docs/api-reference.md` — 전체 Public API 상세 설명
  - `docs/integration-guide.md` — spdlog / glog / 커스텀 통합 방법
  - `docs/ruleset-guide.md` — 룰셋 JSON 작성 및 커스텀 룰 추가 가이드
- [ ] Doxygen 자동 API 문서 생성 설정
- [ ] `README.md` Quick Start 섹션 (5분 안에 동작 확인 가능한 예제 포함)
- [ ] v1.0.0 릴리스

---

## 11. 리스크 및 완화 전략 (Risks & Mitigation)

| 리스크 | 가능성 | 영향 | 완화 전략 |
|---|---|---|---|
| RE2 패턴의 탐지 누락(False Negative) | 중 | 높 | 기본 룰셋 CR(코드 리뷰) + 커버리지 TC 강화 |
| Aho-Corasick 트리거 과탐지로 인한 성능 저하 | 저 | 중 | 트리거 키워드 최소화 지침 문서화 |
| 멀티스레드 RCU 구현 오류로 인한 Data Race | 저 | 높 | TSan(ThreadSanitizer) CI 파이프라인 필수 적용 |
| simdjson API 변경으로 인한 파싱 오류 | 저 | 중 | FetchContent로 버전 고정, 업그레이드 TC 구비 |

---

## 12. 용어 정의 (Glossary)

| 용어 | 설명 |
|---|---|
| **PII** | Personally Identifiable Information. 개인식별정보 (이름, 이메일, 전화번호 등) |
| **Zero-Copy** | 입력 데이터를 복사하지 않고 원본 메모리를 참조하는 처리 방식 |
| **Fast-Path** | 가장 빈번한 케이스(마스킹 불필요)를 위한 최적화된 코드 경로 |
| **RCU** | Read-Copy-Update. 읽기 성능을 극대화하는 Lock-Free 동기화 패턴 |
| **Aho-Corasick** | 여러 키워드를 단일 패스(O(N))로 동시에 탐색하는 문자열 탐색 알고리즘 |
| **Catastrophic Backtracking** | 특정 정규식 패턴에서 입력에 따라 지수 시간으로 느려지는 현상 |
