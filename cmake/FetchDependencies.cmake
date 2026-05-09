include(FetchContent)

# ── abseil-cpp (RE2 의존성) ──────────────────────────────────────────────────
FetchContent_Declare(
    abseil-cpp
    GIT_REPOSITORY https://github.com/abseil/abseil-cpp.git
    GIT_TAG        20240722.0
    GIT_SHALLOW    TRUE
)
set(ABSL_PROPAGATE_CXX_STD ON  CACHE BOOL "" FORCE)
set(ABSL_ENABLE_INSTALL    ON  CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(abseil-cpp)

# ── Google RE2 ──────────────────────────────────────────────────────────────
FetchContent_Declare(
    re2
    GIT_REPOSITORY https://github.com/google/re2.git
    GIT_TAG        2024-03-01
    GIT_SHALLOW    TRUE
)
set(RE2_BUILD_TESTING  OFF CACHE BOOL "" FORCE)
set(RE2_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(re2)

# ── simdjson ────────────────────────────────────────────────────────────────
FetchContent_Declare(
    simdjson
    GIT_REPOSITORY https://github.com/simdjson/simdjson.git
    GIT_TAG        v3.1.0
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(simdjson)

# ── Google Test ─────────────────────────────────────────────────────────────
if(SHIELD_BUILD_TESTS)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG        v1.14.0
        GIT_SHALLOW    TRUE
    )
    set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
endif()

# ── Google Benchmark ────────────────────────────────────────────────────────
if(SHIELD_BUILD_BENCHMARKS)
    FetchContent_Declare(
        benchmark
        GIT_REPOSITORY https://github.com/google/benchmark.git
        GIT_TAG        v1.8.3
        GIT_SHALLOW    TRUE
    )
    set(BENCHMARK_ENABLE_TESTING        OFF CACHE BOOL "" FORCE)
    set(BENCHMARK_ENABLE_INSTALL        OFF CACHE BOOL "" FORCE)
    set(BENCHMARK_DOWNLOAD_DEPENDENCIES OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(benchmark)
endif()
