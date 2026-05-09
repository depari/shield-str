# 고도화 구현: 포맷 유지 마스킹 (Format-Preserving Masking) 적용

**일자:** 2026-05-09  
**작업 내용:** 정규식 캡처 그룹(Captured Groups)을 활용하여 원본 문자열의 일부 형태를 유지하는 Format-Preserving Masking 기능 구현

## 1. 요구사항 및 문제점
- 기존 마스킹 방식은 `mask_group`이 지정되면 해당 그룹을 정적인 `replacement` 문자열(예: `***`)로 1:1 통째로 치환했습니다.
- 보안 감사가 엄격한 시스템에서는 이메일의 호스트명, 전화번호의 앞/뒷자리, 카드번호의 끝 4자리 등을 남겨 데이터 형태를 보존하면서도 식별을 방지하는 **포맷 유지 마스킹(Format-Preserving Masking)** 기능이 필수적으로 요구됩니다.

## 2. TDD 기반 개선 과정
1. **관련 Unit Test 추가 (`test_pattern_matcher.cpp`):**
   - 전화번호 마스킹을 테스트하는 `KoreanPhoneMasking` 케이스를 수정하여, `$1-****-$3` 형태의 교체 문자열을 적용했습니다.
   - 예: `010-1234-5678` -> `010-****-5678` 변환 검증.
2. **개선 전 Test Failed 확인:**
   - 캡처 그룹 치환 엔진이 없어서 리터럴 문자열 `$1-****-$3`이 그대로 출력되어 단위 테스트 실패(Red) 확인.
3. **Format Replacement 로직 구현 및 Test Pass:**
   - `PatternMatcher.cpp`에 `format_replacement` 템플릿 헬퍼 함수를 구현하여 `$1`, `$2` 등의 토큰을 파싱하고 실제 `match` 결과로 치환하는 공통 로직을 작성했습니다.
   - RE2 환경과 `std::regex` 환경 양쪽 모두에서 동일한 문법(`$N`)으로 호환되게 처리하여 일관성을 확보했습니다.
   - 양쪽 빌드 모드 모두에서 `ctest -R KoreanPhoneMasking` 통과(Green) 검증 완료.

## 3. 의의 및 적용
- 이제 외부에서 로드하는 `rules.json`에서 `replacement` 항목에 캡처 그룹 백레퍼런스를 자유롭게 사용할 수 있습니다. (예: `"$1-****-$3"`)
