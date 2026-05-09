# 고도화 구현: 주요 로깅 프레임워크(spdlog) 플러그인(Sink) 지원

**일자:** 2026-05-09  
**작업 내용:** C++ 생태계에서 가장 널리 쓰이는 `spdlog` 라이브러리와 투명하게(Transparent) 연동되는 커스텀 싱크(Sink) 플러그인 개발 및 적용 예제 작성

## 1. 요구사항 및 문제점
- `shield-str` 라이브러리의 코어 엔진을 사용하려면 사용자가 직접 애플리케이션의 곳곳에서 `engine.process(log_str)`를 호출해야 하는 번거로움이 있습니다.
- 현업에서 가장 많이 사용되는 `spdlog`와 완벽히 결합하여, 기존 로깅 코드를 전혀 수정하지 않고도 로그가 출력되기 전 자동으로 마스킹이 적용되는 구조가 필요했습니다. (Transparent Logging)

## 2. 개발 및 검증 과정
1. **커스텀 Sink 래퍼(`SpdlogMaskingSink.hpp`) 구현:**
   - `spdlog::sinks::sink` 인터페이스를 상속받는 `SpdlogMaskingSink` 클래스를 `include/shield/integration/` 하위에 작성했습니다.
   - 프록시(Proxy) 패턴을 활용하여, 원본 `spdlog` 싱크(예: 콘솔, 파일 싱크 등)를 감싸고(Wrap), `log()` 함수가 호출될 때 `shield::MaskingEngine`을 거쳐 필터링된 메시지만 원본 싱크로 전달하도록 오버라이딩했습니다.
2. **연동 예제 및 빌드 구성 (`examples/spdlog_integration`):**
   - `CMakeLists.txt`에 `FetchContent`를 사용하여 `spdlog` (v1.13.0)를 다운로드 받아 빌드하도록 설정했습니다.
   - `main.cpp` 예제에서 일반적인 `spdlog::info()`, `spdlog::warn()`을 호출하면 `password=***` 형태로 자동 치환되어 콘솔에 출력됨을 빌드 및 구동을 통해 검증(Pass)했습니다.

## 3. 의의 및 기대효과
- **코드 수정 제로(Zero Code Modification):** 기존 프로젝트에서 `spdlog`의 기본 로거(Default Logger) 싱크 설정만 `SpdlogMaskingSink`로 한 줄 교체하면, 수십만 줄의 비즈니스 로깅 코드를 일일이 고치지 않아도 완벽한 보안 마스킹 시스템을 구축할 수 있습니다.
- **확장성:** 파일 싱크, 콘솔 싱크, 비동기(Async) 큐 싱크 등 `spdlog`가 지원하는 어떤 싱크와도 조합(Composition)이 가능합니다.
