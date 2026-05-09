add_executable(shield_hot_reload_server main.cpp)
target_link_libraries(shield_hot_reload_server PRIVATE shield::shield-str)
target_include_directories(shield_hot_reload_server PRIVATE ${CMAKE_SOURCE_DIR}/third_party/httplib)

if(APPLE OR UNIX)
    target_link_libraries(shield_hot_reload_server PRIVATE pthread)
endif()
