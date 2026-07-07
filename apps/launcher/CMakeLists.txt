cmake_minimum_required(VERSION 3.22)
project(PhoneVRLauncher VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(PhoneVRLauncher
    src/main.cpp
    src/LauncherWindow.cpp
    src/ServerManager.cpp
)

target_include_directories(PhoneVRLauncher PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(PhoneVRLauncher PRIVATE
    comctl32
    shell32
)
