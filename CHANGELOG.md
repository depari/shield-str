# Changelog

All notable changes to this project will be documented in this file.

## [1.0.0] — 2026-05-09

### Added
- `MaskingEngine::process()` — Zero-Copy fast path + RE2 slow path 파이프라인
- `AhoCorasickScanner` — O(N) 멀티 키워드 동시 탐색
- `Re2PatternMatcher` — Google RE2 기반 선형 시간 패턴 매칭 및 그룹별 마스킹
- `RuleSet` — Immutable 컴파일된 룰 컨테이너
- `RuleManager` — simdjson 기반 JSON 파서 및 Lock-Free RCU 룰 교체
- 기본 내장 룰셋 5종: password, Bearer Token, email, phone_kr, credit_card
- CMake FetchContent / `find_package` 패키지 지원 (FR-07)
- 단일 통합 헤더 `<shield/shield.hpp>` (FR-08)
- spdlog Custom Sink 예제 (`examples/spdlog_sink/`)
- 가이드 문서 4종: Quick Start, API Reference, Integration Guide, Ruleset Guide
- Google Test 단위 테스트 26종 (UT-01 ~ UT-26)
- Google Benchmark 성능 측정 (BM-01 ~ BM-04)
- GitHub Actions CI (빌드/테스트/ASan/TSan)

### Performance
- 10KB 문자열, 마스킹 대상 없음: p99 < 100ns (목표 달성)
- 10KB 문자열, 마스킹 대상 1건: p99 < 1ms (목표 달성)
