cmake_minimum_required(VERSION 3.22)
project(PhoneVRDriver VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find OpenVR SDK
find_path(OPENVR_INCLUDE_DIR
    NAMES openvr.h
    PATHS
        "${CMAKE_CURRENT_SOURCE_DIR}/../../third_party/openvr/headers"
        "${CMAKE_CURRENT_SOURCE_DIR}/../../external/openvr"
        "C:/Program Files (x86)/Steam/steamapps/common/SteamVR/resources/src/openvr"
        "C:/Program Files (x86)/OpenVR"
    PATH_SUFFIXES
        headers
        include
    NO_DEFAULT_PATH
)
find_path(OPENVR_INCLUDE_DIR NAMES openvr.h)

find_library(OPENVR_LIBRARY
    NAMES openvr_api openvr
    PATHS
        "${CMAKE_CURRENT_SOURCE_DIR}/../../third_party/openvr/lib/win64"
        "${CMAKE_CURRENT_SOURCE_DIR}/../../external/openvr/lib/win64"
        "C:/Program Files (x86)/Steam/steamapps/common/SteamVR/resources/src/openvr/bin/win64"
    NO_DEFAULT_PATH
)
find_library(OPENVR_LIBRARY NAMES openvr_api)

if(NOT OPENVR_INCLUDE_DIR OR NOT OPENVR_LIBRARY)
    message(WARNING "OpenVR SDK not found - SteamVR driver will not be built")
    message(WARNING "  OPENVR_INCLUDE_DIR: ${OPENVR_INCLUDE_DIR}")
    message(WARNING "  OPENVR_LIBRARY: ${OPENVR_LIBRARY}")
    return()
endif()

message(STATUS "OpenVR include: ${OPENVR_INCLUDE_DIR}")
message(STATUS "OpenVR library: ${OPENVR_LIBRARY}")

add_library(phonevr_driver SHARED
    src/DriverFactory.cpp
    src/DriverBase.cpp
    src/Devices/HeadsetDevice.cpp
    src/Devices/ControllerDevice.cpp
    src/Input/InputComponent.cpp
    src/Settings/SettingsManager.cpp
)

target_include_directories(phonevr_driver PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${OPENVR_INCLUDE_DIR}
)

target_link_libraries(phonevr_driver PRIVATE
    phonevr_shared
    ${OPENVR_LIBRARY}
)

# Install to SteamVR drivers directory
install(TARGETS phonevr_driver
    RUNTIME DESTINATION ${STEAMVR_DRIVERS_DIR}/phonevr/bin/win64
)

install(FILES
    resources/driver.vrdrivermanifest
    DESTINATION ${STEAMVR_DRIVERS_DIR}/phonevr
)
