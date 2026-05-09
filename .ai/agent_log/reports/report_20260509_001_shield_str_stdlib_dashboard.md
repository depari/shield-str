# 작업 보고서: Shield-Str Stdlib 연동 및 시각화 대시보드 구축

**작업일자:** 2026-05-09  
**작업자:** AI Agent  
**관련 프로젝트:** shield-str

---

## 1. 개요
본 작업은 `shield-str` 라이브러리의 활용성을 극대화하기 위해 다음 두 가지 주요 목표를 달성했습니다.
1. **의존성 분리 (Stdlib-Only 모드):** RE2 및 simdjson과 같은 무거운 외부 라이브러리에 종속되지 않고 C++ 표준 라이브러리(`std::regex`, 기본 문자열 파싱)만으로 라이브러리를 독립 활용할 수 있는 옵션 지원.
2. **성능 시각화 및 대시보드:** 고성능 모드(RE2)와 표준 모드(STD-Only)의 성능 격차를 수치화하고, 사용자가 입력 문자열을 통해 마스킹 전/후 (diff)를 웹 환경에서 직접 테스트해 볼 수 있는 GitHub Pages용 정적 대시보드 구축.

## 2. 작업 상세 내용

### 2.1 Stdlib-Only 모드 구현
- `CMakeLists.txt`에 `-DSHIELD_USE_STD_ONLY=ON` 플래그를 추가하여 `FetchContent`를 통한 RE2 및 simdjson 다운로드 분기 처리.
- `src/PatternMatcher.cpp` 및 `src/RuleSet.cpp` 파일에 컴파일 타임 `#ifdef` 매크로를 적용하여 `std::regex`로 대체되는 Fallback 로직 구현.
- `src/RuleManager.cpp`에 simdjson을 대체하는 단순 구조 기반의 독자적 JSON 파서 추가 (정규식 의존성을 최소화하고 중괄호 카운팅 방식 적용).
- 관련 28개의 단위 테스트 (UT) 및 벤치마크 (BM) 코드가 양쪽 빌드 모드에서 모두 Pass 되도록 정규식 문법 파편화(`(?i)` 플래그 처리 등) 및 메모리 구조 버그 완전 수정.

### 2.2 성능 벤치마크 (Benchmark) 비교
벤치마크 수행 결과 (10KB 로그 데이터 기준), 고성능 정규식 엔진 사용의 이점이 뚜렷하게 확인되었습니다.
- **마스킹 처리 미발생 시 (No-Mask):** 두 모드 모두 초당 **2.9 GB/s**의 고속 처리를 기록 (Aho-Corasick 사전 필터링의 위력 입증).
- **마스킹 발생 시 (With Mask):** 
  - `RE2/simdjson`: **861.4 MB/s**
  - `STD-Only`: **13.1 MB/s**
- **병렬 처리 (Parallel 64-thread):**
  - `RE2/simdjson`: **189.8 k/s** (items/s)
  - `STD-Only`: **95.1 k/s** (items/s)

### 2.3 대시보드(Dashboard) 구축
- `docs/dashboard/` 하위에 HTML, CSS, JavaScript 기반의 정적 웹 페이지 생성.
- **성능 시각화**: 벤치마크된 데이터 수치를 시각적인 비교 카드로 구성.
- **실시간 마스킹 데모 (Diff View)**: 사용자가 입력 패널에 평문 로그를 작성하면, Javascript로 모방 구현된 마스킹 정규식 엔진이 실시간으로 작동하여 마스킹 적용 항목을 하이라이팅하여 출력.

## 3. 제안 (Rule & Guardrail)

TDD 기반 C/C++ 프로젝트를 위해 다음과 같은 룰(Rule)과 하네스(Harness) 업데이트를 제안합니다.

**[Rule 제안: C++ 매크로 기반 다중 빌드 검증 파이프라인]**
- **제안 내용:** `SHIELD_USE_STD_ONLY`와 같은 조건부 의존성이 추가되었을 때, 향후 개발 시 하나의 환경에서만 테스트가 진행되어 다른 환경의 빌드가 깨지는 현상(Regression)이 발생할 위험이 높습니다. 
- **반영 요청:** 프로젝트 룰(가드레일)에 **"조건부 컴파일 플래그가 있는 경우, CI/CD 또는 로컬 검증 단계에서 모든 분기(ON/OFF)에 대한 빌드 및 테스트(ctest)를 필수적으로 교차 검증할 것"**이라는 조항을 추가할 것을 제안합니다.

## 4. 향후 계획
- 개발된 대시보드를 GitHub Pages로 배포 활성화 (사용자 수동 설정 필요).
- 외부 통합을 위한 `api-reference.md` 등 추가 문서화 고도화.
