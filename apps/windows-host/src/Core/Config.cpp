#include "Config.h"
#include <fstream>
#include <sstream>
#include <filesystem>
#include <windows.h>

namespace phonevr {

HostConfig HostConfig::load() {
    HostConfig config;
    std::string path = std::filesystem::current_path().string() + "\\phonevr_config.ini";

    wchar_t wpath[512];
    MultiByteToWideChar(CP_UTF8, 0, path.c_str(), -1, wpath, 512);

    wchar_t buf[256];
    int result;

    result = GetPrivateProfileStringW(L"PhoneVR", L"listen_address", L"0.0.0.0",
                                       buf, 256, wpath);
    if (result > 0) {
        char addr[64];
        WideCharToMultiByte(CP_UTF8, 0, buf, -1, addr, 64, nullptr, nullptr);
        config.listen_address = addr;
    }

    config.control_port = GetPrivateProfileIntW(L"PhoneVR", L"control_port",
                                                  config.control_port, wpath);
    config.video_port = GetPrivateProfileIntW(L"PhoneVR", L"video_port",
                                                config.video_port, wpath);
    config.render_width = GetPrivateProfileIntW(L"PhoneVR", L"render_width",
                                                  config.render_width, wpath);
    config.render_height = GetPrivateProfileIntW(L"PhoneVR", L"render_height",
                                                   config.render_height, wpath);
    config.target_fps = GetPrivateProfileIntW(L"PhoneVR", L"target_fps",
                                                config.target_fps, wpath);
    config.target_bitrate = GetPrivateProfileIntW(L"PhoneVR", L"target_bitrate",
                                                    config.target_bitrate, wpath);

    wchar_t log_buf[32];
    result = GetPrivateProfileStringW(L"PhoneVR", L"log_level", L"info",
                                       log_buf, 32, wpath);
    if (result > 0) {
        char level[32];
        WideCharToMultiByte(CP_UTF8, 0, log_buf, -1, level, 32, nullptr, nullptr);
        config.log_level = level;
    }

    return config;
}

void HostConfig::save() const {
    std::string path = std::filesystem::current_path().string() + "\\phonevr_config.ini";

    wchar_t wpath[512];
    MultiByteToWideChar(CP_UTF8, 0, path.c_str(), -1, wpath, 512);

    wchar_t buf[64];

    swprintf(buf, 64, L"%d", control_port);
    WritePrivateProfileStringW(L"PhoneVR", L"control_port", buf, wpath);

    swprintf(buf, 64, L"%d", video_port);
    WritePrivateProfileStringW(L"PhoneVR", L"video_port", buf, wpath);

    swprintf(buf, 64, L"%d", render_width);
    WritePrivateProfileStringW(L"PhoneVR", L"render_width", buf, wpath);

    swprintf(buf, 64, L"%d", render_height);
    WritePrivateProfileStringW(L"PhoneVR", L"render_height", buf, wpath);

    swprintf(buf, 64, L"%d", target_fps);
    WritePrivateProfileStringW(L"PhoneVR", L"target_fps", buf, wpath);

    swprintf(buf, 64, L"%d", target_bitrate);
    WritePrivateProfileStringW(L"PhoneVR", L"target_bitrate", buf, wpath);
}

} // namespace phonevr
