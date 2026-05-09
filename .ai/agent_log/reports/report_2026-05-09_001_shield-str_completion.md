# shield-str 개발 및 안정화 완료 보고서

## 1. 개요
* **프로젝트명**: shield-str (Zero-Copy C++17 로그 마스킹 엔진)
* **작업 일시**: 2026년 5월 9일
* **목표**: RE2 및 simdjson 연동 중 발생한 빌드/링크 문제 해결, 전체 단위 테스트(28개) 통과, 및 개발 완료.

## 2. 발생했던 문제점 및 해결 과정
1. **simdjson 컴파일 및 링크 에러**
   * **원인**: AppleClang 컴파일러와 simdjson의 DOM(Document Object Model) 직렬화 API 간 C++ 표준 해석 차이, 및 CMake FetchContent 시 정적 라이브러리의 심볼 누락 현상 발생.
   * **개선 및 해결**:
     1. 관련 unit test (`test_rule_manager.cpp`) 준비 및 실패 확인.
     2. simdjson 라이브러리를 최신(v3.9.1)에서 안정화된 v3.1.0 버전으로 다운그레이드.
     3. 헤더 인클루드를 `<simdjson/ondemand.h>`에서 singleheader 방식의 `"simdjson.h"`로 통합하여 소스 단위 병합.
     4. 빌드 후 전체 TC 통과 확인.

2. **단위 테스트 (UT-06) Assertion 로직 오류**
   * **원인**: 정규식 마스킹 결과에서 원본 문자가 남지 않음을 검증하는 로직에 잘못된 연산자(`EXPECT_NE`와 삼항 연산자의 오용)가 사용됨.
   * **해결**: `EXPECT_EQ(result->find("secret123"), std::string::npos);`로 직관적이고 정확한 단언문으로 리팩토링 후 테스트 100% 성공 확인.

3. **기본 예제(basic_usage) Raw String Literal 이스케이프 오류**
   * **원인**: JSON 문자열을 하드코딩할 때 C++의 `R"json(...)json"` 구문 대신 일반 문자열 내 이스케이프(`\\`) 오류로 컴파일 실패 발생.
   * **해결**: 모든 JSON 및 정규식 하드코딩 변수를 Raw String Literal로 변경하여 안전하게 컴파일되도록 수정.

## 3. 결과 및 검증
* **전체 단위 테스트 성공**: 총 28개의 테스트 케이스(Aho-Corasick, RE2 패턴 매칭, RCU 기반 스레드 안전성 등)가 100% 통과 (0.40 sec 수행 완료).
* **예제 동작 확인**: `shield_basic_usage` 실행 결과, Fast-path(Zero-copy 모드)와 Slow-path(마스킹 모드) 모두 정확히 동작함을 증명.
* **핫 릴로드(Hot Rule Update)**: 프로그램 재시작 없이 런타임 중 마스킹 룰이 `std::shared_mutex`를 이용한 Lock-Free 기반으로 무중단 교체되는 것 검증 완료.

## 4. Rule 제안
작업을 진행하며 프로젝트 고도화를 위해 다음과 같은 추가 Rule 도입을 제안합니다.
* **Rule 제안 사항**: **`[의존성 관리]`**: 외부 C++ 헤더 전용(Header-only) 또는 Single-header 라이브러리(예: simdjson, catch2 등)를 사용할 때는 FetchContent로 매번 빌드하기보다는 프로젝트의 `third_party/` 디렉토리에 스냅샷 형태로 고정하여 외부 네트워크 장애 및 컴파일러 업데이트에 따른 갑작스러운 빌드 실패(Regression)를 방지할 것을 제안합니다.
* **추가 하네스(Harness) 제안**: CI/CD 파이프라인에서 CMake의 `-Werror` (경고를 에러로 취급) 플래그를 추가하여 컴파일러 경고 단계를 원천 차단하는 Verification 단계를 업데이트할 것을 제안합니다.
