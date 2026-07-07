#include "ServerManager.h"
#include <iostream>

namespace phonevr {

bool ServerManager::start_host_process(const std::string& host_path) {
    if (host_running_) return true;

    std::string cmd = "\"" + host_path + "\"";

    STARTUPINFOA si = {};
    si.cb = sizeof(STARTUPINFOA);

    BOOL result = CreateProcessA(
        nullptr,
        const_cast<char*>(cmd.c_str()),
        nullptr, nullptr, FALSE, 0, nullptr, nullptr,
        &si, &host_process_);

    if (result) {
        host_running_ = true;
        if (on_host_status_change) on_host_status_change(true);
        return true;
    }

    return false;
}

bool ServerManager::stop_host_process() {
    if (!host_running_) return true;

    if (TerminateProcess(host_process_.hProcess, 0)) {
        CloseHandle(host_process_.hProcess);
        CloseHandle(host_process_.hThread);
        host_running_ = false;
        if (on_host_status_change) on_host_status_change(false);
        return true;
    }

    return false;
}

bool ServerManager::start_steamvr_driver(const std::string& driver_path) {
    // SteamVR drivers are loaded by SteamVR itself
    // We just need to ensure SteamVR is running
    STARTUPINFOA si = {};
    si.cb = sizeof(STARTUPINFOA);
    PROCESS_INFORMATION pi = {};

    // Try to find SteamVR
    const char* steam_path = getenv("ProgramFiles(x86)");
    if (!steam_path) {
        steam_path = getenv("ProgramFiles");
    }

    if (steam_path) {
        std::string vr_path = std::string(steam_path) +
            "\\Steam\\steamapps\\common\\SteamVR\\bin\\win64\\vrstartup.exe";

        return CreateProcessA(const_cast<char*>(vr_path.c_str()),
                               nullptr, nullptr, nullptr, FALSE, 0,
                               nullptr, nullptr, &si, &pi) == TRUE;
    }

    return false;
}

bool ServerManager::is_host_running() const {
    return host_running_;
}

} // namespace phonevr
