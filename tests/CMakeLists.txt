cmake_minimum_required(VERSION 3.22)
project(PhoneVRTests VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(shared_test
    shared/test_crc32.cpp
)

add_executable(windows_test
    windows/test_network.cpp
)

add_executable(steamvr_test
    steamvr/test_driver.cpp
)

target_include_directories(shared_test PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../shared/cpp
)

target_include_directories(windows_test PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../shared/cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../apps/windows-host/include
)

target_include_directories(steamvr_test PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../shared/cpp
)

# Enable testing
enable_testing()
add_test(NAME SharedTests COMMAND shared_test)
add_test(NAME WindowsTests COMMAND windows_test)
add_test(NAME SteamVRTests COMMAND steamvr_test)
