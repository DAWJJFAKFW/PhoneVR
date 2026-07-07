#pragma once

#include <windows.h>
#include <string>
#include <functional>

namespace phonevr {

class ServerManager {
public:
    ServerManager() = default;
    ~ServerManager() = default;

    bool start_host_process(const std::string& host_path);
    bool stop_host_process();
    bool start_steamvr_driver(const std::string& driver_path);
    bool is_host_running() const;

    std::function<void(bool)> on_host_status_change;

private:
    PROCESS_INFORMATION host_process_ = {};
    bool host_running_ = false;
};

} // namespace phonevr
