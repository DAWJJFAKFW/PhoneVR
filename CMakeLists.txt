cmake_minimum_required(VERSION 3.22)
project(PhoneVR VERSION 1.0.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Options
option(BUILD_WINDOWS_HOST "Build Windows Host" ON)
option(BUILD_STEAMVR_DRIVER "Build SteamVR Driver" ON)
option(BUILD_LAUNCHER "Build Launcher" ON)
option(BUILD_TESTS "Build Tests" ON)

# Shared library
add_subdirectory(shared/cpp)

# Apps
if(BUILD_WINDOWS_HOST)
    add_subdirectory(apps/windows-host)
endif()

if(BUILD_STEAMVR_DRIVER)
    add_subdirectory(apps/steamvr-driver)
endif()

if(BUILD_LAUNCHER)
    add_subdirectory(apps/launcher)
endif()

# Tests
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
