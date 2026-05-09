# 고도화 구현: 동적 룰셋 주입 API 연동 (Hot Reload)

**일자:** 2026-05-09  
**작업 내용:** `cpp-httplib`을 활용하여 백그라운드 프로세스 중단 없이 실시간으로 JSON 형태의 마스킹 룰을 업데이트(Hot Swap)하는 기능 구현

## 1. 요구사항 및 문제점
- 서비스가 가동 중인 환경(예: 웹 서버, 백엔드 데몬 등)에서 새로운 민감정보 패턴이나 룰 변경이 생겼을 때, 애플리케이션을 재배포하거나 재시작하는 것은 운영상 부담이 됩니다.
- 이에 `RuleManager::update_rules` 내부의 Read-Copy-Update(RCU) 메커니즘을 외부 시스템(API)과 연동하는 예제 및 구조 검증이 필요했습니다.

## 2. 개발 및 검증 과정
1. **의존성 추가 (Vendor화):**
   - 네트워크 연결 문제 등을 예방하기 위해, 가벼운 단일 헤더 라이브러리인 `cpp-httplib` (v0.15.3)을 `third_party/httplib` 폴더에 로컬 내장(Vendor)하였습니다. (README.md 문서 명시 완료)
2. **핫 리로드 서버 예제 작성 (`examples/hot_reload_server/main.cpp`):**
   - 백그라운드 스레드에서 지속적으로 로그를 생성하는 환경을 구성.
   - HTTP POST `/api/rules` 엔드포인트를 열어, 요청 바디로 들어오는 JSON 룰셋을 받아 즉시 `mgr->update_rules()`를 호출하도록 구현했습니다.
3. **빌드 및 실제 구동 테스트 (Test Pass):**
   - `cmake` 설정 시 `examples/hot_reload_server` 디렉토리를 포함하도록 `CMakeLists.txt`를 업데이트했습니다.
   - 서버를 가동한 뒤, 초기에는 비밀번호가 그대로 출력(`password=secret_pw123!`)되는 것을 확인했습니다.
   - `curl -X POST http://localhost:8080/api/rules -d @config/rules.json` 명령어를 통해 실시간으로 룰셋을 주입하자, 다음 줄부터 즉시 `password=***` 형태로 마스킹이 동작함을 검증 완료했습니다.

## 3. 의의 및 적용 방안
- 기존의 RCU 기반 Lock-free 아키텍처가 동시성 이슈 없이 완벽히 동작함을 외부 API 연동을 통해 증명했습니다.
- 이를 응용하여, 사용자는 중앙 집중식 관리 서버나 Admin UI에서 마스킹 정책을 변경하고, 구동 중인 수많은 마이크로서비스 노드들에 룰셋을 실시간 동기화(Push)할 수 있습니다.
