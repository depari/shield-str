include(FetchContent)

FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG        v1.13.0
)
FetchContent_MakeAvailable(spdlog)

add_executable(shield_spdlog_integration main.cpp)
target_link_libraries(shield_spdlog_integration PRIVATE shield::shield-str spdlog::spdlog)
