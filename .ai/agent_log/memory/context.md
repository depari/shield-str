# 프로젝트 컨텍스트 (2026-05-09)

## 프로젝트 상태
- **멀티 엔진 지원**: RE2, PCRE2, STD(std::regex) 엔진 통합 완료.
- **빌드 시스템**: CMake를 통해 엔진 선택 가능 (`-DSHIELD_REGEX_ENGINE=PCRE2/RE2/STD`).
- **안정성**: `ThreadSafetyTest` 및 캡처 그룹 마스킹 로직 안정화 완료.
- **JSON 파싱**: `simdjson` 기반 고성능 파싱 및 `STD-ONLY` 모드를 위한 수동 파싱 지원.

## 주요 구성 요소
- `MaskingEngine`: 전체 마스킹 흐름 제어 (Scanner -> Matcher).
- `RuleManager`: JSON 설정 로드 및 `RuleSet` 관리 (핫 리로드 지원).
- `PatternMatcher`: 실제 정규식 매칭 및 치환 수행 (엔진별 특화 로직).
- `PCRE2Wrapper`: PCRE2 엔진 연동을 위한 래퍼 클래스.

## 향후 과제
- 벤치마크 데이터 시각화 및 대시보드 구축.
- `STD-ONLY` 모드에서의 JSON 파서 고도화.
- 대규모 트래픽 환경에서의 성능 튜닝.
