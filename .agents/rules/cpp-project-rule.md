---
trigger: always_on
---

[의존성관리]
  # C/C++ 프로젝트 외부 라이브러리 연동 Rule
    - simdjson, Catch2 등 Single-header 형태나 가벼운 Header-only 라이브러리는 FetchContent를 통한 실시간 다운로드 방식 대신, 프로젝트의 `third_party/` 폴더에 물리적으로 다운로드하여 내장(Vendor)시키는 방식을 최우선으로 고려 해.
    - 외부 네트워크 단절 상황이나 upstream 저장소의 갑작스러운 태그 변경(삭제), 컴파일러 업데이트에 따른 빌드 실패(Regression)를 방지하기 위함이야.
    - 의존성을 추가할 때는 반드시 `third_party/[패키지명]/README.md`에 해당 패키지의 버전(Tag)과 출처(URL)를 명시 해.

[Harness]
  # 빌드 안정성 확보 Harness
    - C++ 빌드 파이프라인 구성 시 (CMake, Make 등), 잠재적 크래시 및 호환성 문제를 사전에 차단하기 위해 컴파일러 경고를 에러로 취급하는 `-Werror` (MSVC의 경우 `/WX`) 플래그를 로컬 테스트 및 CI/CD 빌드 옵션에 반드시 포함 해.
    - CMakeLists.txt 작성 시 아래 코드를 표준 하네스로 반영 해:
      `target_compile_options(${PROJECT_NAME} PRIVATE $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wpedantic -Werror>)`
    - 만약 특정 외부 라이브러리 때문에 `-Werror`로 빌드가 불가능한 경우, 해당 라이브러리의 include는 `-isystem`을 사용하여 경고를 무시하도록 처리 해.

[다중 빌드 환경 검증]
  # 조건부 의존성(Stdlib-only 등) 컴파일 검증 Rule
    - 프로젝트 내에 빌드 환경을 분기하는 플래그(예: `SHIELD_USE_STD_ONLY`)가 도입될 경우, 특정 환경에서만 컴파일이 성공하고 다른 환경에서는 빌드가 깨지는 Regression을 방지해야 해.
    - 따라서 향후 기능 구현 및 리팩토링 시에는 CI/CD 또는 로컬 검증 단계에서 모든 분기(예: Default 모드 및 Stdlib-Only 모드)에 대한 빌드 및 테스트(ctest)를 필수적으로 교차 검증 해.
