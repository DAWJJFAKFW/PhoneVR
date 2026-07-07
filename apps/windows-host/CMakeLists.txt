cmake_minimum_required(VERSION 3.22)
project(PhoneVRHost VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(PhoneVRHost
    src/main.cpp
    src/Core/Config.cpp
    src/Core/StatsCollector.cpp
    src/Network/NetworkReceiver.cpp
    src/Network/StreamingProtocol.cpp
    src/Video/HardwareDecoder.cpp
    src/Video/FrameProcessor.cpp
    src/Render/D3D12Renderer.cpp
    src/Render/StereoRenderTarget.cpp
    src/OpenXR/OpenXRIntegration.cpp
    src/OpenVR/OpenVRIntegration.cpp
)

target_include_directories(PhoneVRHost PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(PhoneVRHost PRIVATE
    phonevr_shared
    ws2_32
    mfplat
    mfuuid
    mfreadwrite
    d3d12
    dxgi
    d3dcompiler
    dxguid
    OpenXR_loader
)

# Find OpenXR
find_path(OPENXR_INCLUDE_DIR OpenXR/openxr.h)
if(OPENXR_INCLUDE_DIR)
    target_include_directories(PhoneVRHost PRIVATE ${OPENXR_INCLUDE_DIR})
    add_definitions(-DSUPPORT_OPENXR)
endif()

# Find SteamVR
find_path(OPENVR_INCLUDE_DIR openvr/openvr.h)
if(OPENVR_INCLUDE_DIR)
    target_include_directories(PhoneVRHost PRIVATE ${OPENVR_INCLUDE_DIR})
    add_definitions(-DSUPPORT_STEAMVR)
endif()

# FFmpeg (optional)
find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
    pkg_check_modules(FFMPEG QUIET libavcodec libavformat libavutil libswscale)
    if(FFMPEG_FOUND)
        target_include_directories(PhoneVRHost PRIVATE ${FFMPEG_INCLUDE_DIRS})
        target_link_libraries(PhoneVRHost PRIVATE ${FFMPEG_LIBRARIES})
        add_definitions(-DUSE_FFMPEG)
    endif()
endif()
