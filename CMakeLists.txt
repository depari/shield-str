cmake_minimum_required(VERSION 3.16)
project(shield-str VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ── Options ──────────────────────────────────────────────────────────────────
option(SHIELD_BUILD_TESTS       "Build unit tests"        ON)
option(SHIELD_BUILD_BENCHMARKS  "Build benchmarks"        ON)
option(SHIELD_BUILD_EXAMPLES    "Build examples"          ON)
option(SHIELD_INSTALL           "Install shield-str"      ON)
option(SHIELD_USE_STD_ONLY      "Use std::regex and manual JSON parser instead of RE2/simdjson" OFF)

# ── Dependencies ─────────────────────────────────────────────────────────────
include(cmake/FetchDependencies.cmake)

# ── Library target ───────────────────────────────────────────────────────────
add_library(shield-str
    src/AhoCorasickScanner.cpp
    src/PatternMatcher.cpp
    src/RuleSet.cpp
    src/RuleManager.cpp
    src/MaskingEngine.cpp
)

if(NOT SHIELD_USE_STD_ONLY)
    target_sources(shield-str PRIVATE ${simdjson_SOURCE_DIR}/singleheader/simdjson.cpp)
endif()

add_library(shield::shield-str ALIAS shield-str)

if(SHIELD_USE_STD_ONLY)
    target_compile_definitions(shield-str PUBLIC SHIELD_USE_STD_ONLY)
else()
    target_link_libraries(shield-str PUBLIC re2::re2)
    target_include_directories(shield-str PRIVATE ${simdjson_SOURCE_DIR}/singleheader)
endif()

target_include_directories(shield-str
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_compile_options(shield-str PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wpedantic>
)

# ── Tests ────────────────────────────────────────────────────────────────────
if(SHIELD_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# ── Benchmarks ───────────────────────────────────────────────────────────────
if(SHIELD_BUILD_BENCHMARKS)
    add_subdirectory(tests/bench)
endif()

# ── Examples ─────────────────────────────────────────────────────────────────
if(SHIELD_BUILD_EXAMPLES)
    add_subdirectory(examples/basic_usage)
    add_subdirectory(examples/hot_reload_server)
endif()

# ── Install / find_package support ───────────────────────────────────────────
if(SHIELD_INSTALL)
    include(GNUInstallDirs)
    include(CMakePackageConfigHelpers)

    install(TARGETS shield-str
        EXPORT shield-str-targets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )

    install(DIRECTORY include/shield
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )

    install(EXPORT shield-str-targets
        FILE        shield-str-targets.cmake
        NAMESPACE   shield::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/shield-str
    )

    configure_package_config_file(
        cmake/shield-str-config.cmake.in
        ${CMAKE_CURRENT_BINARY_DIR}/shield-str-config.cmake
        INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/shield-str
    )

    write_basic_package_version_file(
        ${CMAKE_CURRENT_BINARY_DIR}/shield-str-config-version.cmake
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY SameMajorVersion
    )

    install(FILES
        ${CMAKE_CURRENT_BINARY_DIR}/shield-str-config.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/shield-str-config-version.cmake
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/shield-str
    )
endif()
