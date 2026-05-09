# 문제점 개선: AuthToken 및 AuthCode 포맷 마스킹 패턴 추가 및 룰 외부화

**일자:** 2026-05-09  
**작업 내용:** `authtoken=`, `authcode:` 등 인증 토큰 키워드 대응 및, 외부에서 룰셋을 로드할 수 있도록 `config/rules.json` 파일 구성

## 1. 문제 원인 파악 및 요구사항
- 기존 룰은 `password`, `Bearer` 등 주요 패턴만 하드코딩 되어 있었고, `authtoken=...` 이나 `authcode:` 등 실무에서 자주 등장하는 포맷에 대한 마스킹 패턴이 누락되어 있었습니다.
- 또한, 룰을 소스 내에서만 관리하지 않고 별도의 파일로 관리하여 쉽게 확장할 수 있도록 하는 아키텍처 개선 요구사항이 접수되었습니다.

## 2. TDD 기반 개선 및 기능 추가
1. **외부 룰 관리 파일 구성:**
   - `config/rules.json` 파일을 신규 생성하여, 기본 룰(패스워드, 이메일, 전화번호 등)과 새로 추가할 AuthToken 패턴을 통합 등록했습니다. 
   - `RuleManager::from_file("config/rules.json")` API를 통해 언제든 외부에서 JSON을 로드하여 엔진에 주입할 수 있는 구조입니다 (이전 버전에서 구현된 API 활용).
2. **관련 Unit Test 생성 및 패턴 검증:** 
   - `tests/unit/test_masking_engine.cpp` 에 `AuthTokenMasking` (UT-26) 테스트 추가.
   - `authtoken=ABC123456XYZ` 와 `authcode : secret_token_99` 형태의 입력을 검증하여, 실패를 확인(Red) 후, 정규식 패턴 `(?i)(?:authtoken|authcode|auth_token|auth_code)["']?\s*[:=]\s*["']?([^\s"']+)`를 반영하여 통과(Green) 처리함.
3. **대시보드 Live Mode 반영:**
   - 대시보드 UI의 Active Rules 목록에 새로 추가된 AuthToken/AuthCode 정규식 표시.
   - `Plain`, `JSON`, `Colon` 3가지 예제 데이터 세트에 모두 AuthToken 마스킹이 동작하는 샘플 문자열을 삽입.
