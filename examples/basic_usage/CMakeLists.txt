cmake_minimum_required(VERSION 3.16)

add_executable(shield_basic_usage main.cpp)
target_link_libraries(shield_basic_usage PRIVATE shield::shield-str)
