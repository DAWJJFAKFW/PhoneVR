cmake_minimum_required(VERSION 3.22)

add_executable(shared_test
    shared/test_crc32.cpp
)

add_executable(windows_test
    windows/test_network.cpp
)

add_executable(steamvr_test
    steamvr/test_driver.cpp
)

target_link_libraries(shared_test PRIVATE phonevr_shared)
target_link_libraries(windows_test PRIVATE phonevr_shared)
target_link_libraries(steamvr_test PRIVATE phonevr_shared)

target_include_directories(windows_test PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../apps/windows-host/include
)

add_test(NAME SharedTests COMMAND shared_test)
add_test(NAME WindowsTests COMMAND windows_test)
add_test(NAME SteamVRTests COMMAND steamvr_test)
