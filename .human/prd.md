# 📄 Product Requirements Document (PRD)
**프로젝트명:** `shield-str` (High-Performance C++ Log Masking Utility)
**문서 버전:** 1.0.0
**작성일:** 2026-05-09

## 1. 프로젝트 개요 (Overview)
`shield-str`은 C++ 애플리케이션에서 발생하는 대용량 로그(JSON, Plain Text 등)가 콘솔이나 로그 수집 서버로 출력되기 전, Password, Token, PII(개인식별정보) 등의 민감 데이터를 식별하고 마스킹(***) 처리하는 초고속 유틸리티 라이브러리입니다. 로깅 과정에서 발생하는 성능 오버헤드를 제로(0)에 가깝게 유지하면서도, 런타임에 유연하게 마스킹 규칙을 업데이트할 수 있도록 설계됩니다.

## 2. 목표 및 배경 (Objectives & Background)
*   **배경:** 시스템 모니터링과 디버깅을 위해 상세한 로그 출력이 필수적이나, 이 과정에서 사용자 개인정보나 시스템 보안 토큰이 노출되는 보안 사고가 빈번히 발생합니다.
*   **목표:** 
    *   기존 로깅 파이프라인(예: spdlog, glog 등)에 쉽게 통합할 수 있는 독립적인 C++ 모듈 제공.
    *   초당 수만 건의 텍스트가 쏟아지는 환경에서도 로깅 스레드에 병목을 일으키지 않는 극강의 처리 속도 달성.
    *   애플리케이션 재시작 없이 보안 정책 변경(마스킹 룰 추가/수정) 사항을 즉각 반영.

## 3. 기능 요구사항 (Functional Requirements)

| 요구사항 ID | 기능명 | 설명 |
| :--- | :--- | :--- |
| **FR-01** | **문자열 마스킹 파이프라인** | 입력된 긴 로그 문자열에서 사전에 정의된 민감 정보 패턴을 찾아 지정된 문자(예: `*`)로 치환하여 반환한다. |
| **FR-02** | **다중 포맷 지원** | Plain Text 내의 키워드 주변 값 식별 및 JSON 구조 내의 특정 Key 값 마스킹을 모두 지원한다. |
| **FR-03** | **기본 내장 룰셋 제공** | 이메일, 전화번호, 범용 Auth Token(Bearer), Password 등에 대한 기본 정규식 템플릿을 내장한다. |
| **FR-04** | **동적 룰 로딩** | JSON 또는 YAML 형식의 설정 파일로부터 마스킹 룰(키워드, 정규식, 치환 문자열)을 파싱하여 시스템에 적용한다. |

## 4. 비기능 요구사항 (Non-Functional Requirements) 🌟 (핵심)

### 4.1 성능 (Performance) - **가장 높은 우선순위**
*   **NFR-01 (Zero-Copy 기반 스캐닝):** 입력 문자열은 복사 없이 `std::string_view` 형태로 엔진에 전달되어야 하며, 불필요한 메모리 할당(Heap Allocation)을 엄격히 금지한다. 마스킹이 발생할 때만 원본 사이즈를 기반으로 `std::string::reserve()`를 호출해 버퍼를 생성한다.
*   **NFR-02 (Fast-Path Early Exit):** Aho-Corasick 알고리즘을 적용하여 입력 문자열 내에 마스킹 대상 '트리거 키워드'가 존재하는지 `O(N)` 시간 복잡도로 일괄 탐색한다. 키워드가 없는 경우 정규식 엔진을 호출하지 않고 원본 `string_view`를 즉시 반환하여 성능 저하를 방지한다.
*   **NFR-03 (고성능 정규식 엔진):** `std::regex` 사용을 배제하고, 선형 시간(Linear Time) 탐색을 보장하는 Google **RE2** 라이브러리를 채택하여 파국적 역추적(Catastrophic Backtracking)으로 인한 CPU 스파이크를 원천 차단한다.
*   **목표 지표:** 10KB 길이의 일반 문자열(마스킹 대상 없음) 처리 시 지연 시간 `100ns` 이하, 마스킹 대상 존재 시 `1ms` 이하 보장.

### 4.2 유지보수성 및 확장성 (Maintainability & Extensibility)
*   **NFR-04 (무중단 룰 업데이트 - RCU 패턴):** 런타임에 외부 설정 파일이 변경되거나 API 호출로 룰 업데이트가 요청될 경우, 멀티스레드 환경에서 Lock 경합을 유발하지 않아야 한다. `std::atomic<std::shared_ptr<RuleSet>>`을 활용한 **Read-Copy-Update(RCU)** 패턴을 구현하여, 로깅 스레드의 대기(Blocking) 없이 안전하게 룰 객체를 스왑(Swap)한다.

### 4.3 호환성 및 제약사항 (Constraints)
*   **NFR-05 (C++ 표준):** `std::string_view` 및 멀티스레딩 최적화를 위해 **C++17** 이상을 표준으로 작성한다.
*   **NFR-06 (의존성 최소화):** 핵심 의존성은 `RE2` (정규식)와 `simdjson` (초고속 룰셋 파싱) 두 가지로 제한하며, 빌드 시스템은 `CMake`를 사용한다.

## 5. 시스템 아키텍처 및 모듈 구성 (Architecture)

1.  **`shield::MaskingEngine`**: 라이브러리의 진입점(Entry Point). `std::string_view`를 입력받아 최종 결과를 반환하는 메인 파이프라인.
2.  **`shield::RuleManager`**: `simdjson`을 이용해 외부 룰 설정을 파싱하고, `RuleSet` 객체의 생명주기와 원자적(Atomic) 교체를 관리.
3.  **`shield::RuleSet` (Immutable)**: 읽기 전용 객체로 구성되며 아래의 엔진들을 소유함.
    *   **`AhoCorasickScanner`**: 설정된 모든 룰의 '키워드'를 트리 구조로 미리 컴파일해 둔 고속 스캐너.
    *   **`Re2PatternMatcher`**: 런타임 정규식 컴파일 비용을 없애기 위해 미리 컴파일된 `re2::RE2` 객체들의 집합.

## 6. 마일스톤 및 릴리스 계획 (Milestones)

*   **Phase 1: Core Engine & Fast-Path**
    *   CMake 환경 구성 및 `RE2` 연동.
    *   Aho-Corasick 구현 및 `std::string_view` 기반 스캐닝 파이프라인 구축.
    *   단위 테스트 (Google Test) 및 성능 Baseline 측정 (Google Benchmark).
*   **Phase 2: Dynamic Rules & Thread Safety**
    *   `simdjson` 연동 및 JSON 룰셋 파서 구현.
    *   `std::atomic`을 활용한 Lock-Free 룰셋 스왑(RCU 패턴) 구현 및 멀티스레드 스트레스 테스트.
*   **Phase 3: Integration & Optimization**
    *   주요 C++ 로깅 프레임워크(spdlog) 커스텀 싱크(Custom Sink) 예제 작성.
    *   메모리 누수 검증(Valgrind) 및 CPU 프로파일링. 배포 준비.