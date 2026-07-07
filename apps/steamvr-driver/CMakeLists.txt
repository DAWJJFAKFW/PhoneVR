cmake_minimum_required(VERSION 3.22)
project(PhoneVRDriver VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

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
    openvr_api
)

# Install to SteamVR drivers directory
install(TARGETS phonevr_driver
    RUNTIME DESTINATION ${STEAMVR_DRIVERS_DIR}/phonevr/bin/win64
)

install(FILES
    resources/driver.vrdrivermanifest
    DESTINATION ${STEAMVR_DRIVERS_DIR}/phonevr
)
