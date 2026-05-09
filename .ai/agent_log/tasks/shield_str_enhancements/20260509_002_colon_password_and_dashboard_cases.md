# 문제점 개선: Colon(:) 기반 비밀번호 포맷 마스킹 누락 대응 및 대시보드 고도화

**일자:** 2026-05-09  
**작업 내용:** `password:eondlkajsdfasd` 형태의 문자열에 대한 마스킹 처리 개선 및 대시보드 시나리오(Examples) 다각화 반영

## 1. 문제 원인 파악
기존의 비밀번호 마스킹 정규식 패턴(`(?i)password=(\\S+)`)은 오직 등호(`=`) 기호가 포함된 경우만 처리하도록 하드코딩 되어 있었습니다. 이로 인해 `password:eondlkajsdfasd` (Colon 기호 사용) 또는 `"password": "secret"` 과 같은 JSON 형식의 로깅에서는 정규식 매칭에 실패하여 마스킹이 누락되는 보안 결함(문제점)이 발생했습니다.

## 2. TDD 기반 개선 과정
1. **관련 Unit Test 생성:** 
   - `tests/unit/test_masking_engine.cpp` 내에 `PasswordWithColonMasking` (UT-25) 테스트 케이스를 신규 작성하여, `password:eondlkajsdfasd` 형태가 마스킹 되는지 검증하는 로직 추가.
2. **개선 전 Test Failed 확인:**
   - 빌드 후 `ctest -R PasswordWithColonMasking` 실행 결과, 예상대로 마스킹이 되지 않아(Zero-copy fallback) 테스트 실패(`Failed`) 확인.
3. **정규식 고도화 및 Test Pass 확인:**
   - 등호뿐만 아니라 콜론 및 따옴표 기호를 범용적으로 커버할 수 있도록 패턴을 개선함: `(?i)(?:password|passwd|pwd)["']?\s*[:=]\s*["']?([^\s"']+)`
   - `test_rule_manager.cpp`, `test_pattern_matcher.cpp`, `test_thread_safety.cpp` 및 각종 벤치마크 파일(BM)에 정의된 기본 룰셋 JSON의 정규식 일괄 교체 수행.
   - 다시 `ctest`를 실행하여 100% Pass 됨을 검증 완료.

## 3. 대시보드 고도화 (사례 비교 뷰 추가)
- **Live Mode 확장:** 단순히 직접 입력하는 모드 외에, 사용자가 손쉽게 C++ 엔진의 성능 및 대응성을 체험할 수 있도록 `Plain`, `JSON`, `Colon` 3가지 기본 프리셋 예제 로드 버튼(`loadExample()`)을 UI에 추가했습니다.
- 이를 통해 새로 개선된 정규식 로직이 JSON 포맷 및 Colon 포맷에 어떻게 매칭되어 동작하는지 즉각적으로 시각화(Diff) 할 수 있게 됨.
