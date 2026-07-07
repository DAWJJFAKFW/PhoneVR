#pragma once

#include <openvr_driver.h>
#include <memory>
#include <string>
#include <thread>
#include <atomic>

#include "Devices/HeadsetDevice.h"
#include "Devices/ControllerDevice.h"
#include "Input/InputComponent.h"
#include "Settings/SettingsManager.h"

namespace phonevr {

class DriverBase : public vr::IDriverBase {
public:
    DriverBase();
    ~DriverBase() override;

    // IDriverBase
    vr::EVRInitError Init(vr::IDriverLog* pDriverLog,
                          vr::IDriverServer* pDriverServer,
                          vr::IDriverHost* pDriverHost,
                          const char* pchUserDriverConfigDir,
                          const char* pchDriverInstallDir) override;
    void Cleanup() override;
    uint32_t GetTrackedDeviceCount() override;
    vr::ITrackedDeviceServerDriver* GetTrackedDevice(uint32_t unWhich) override;
    vr::ITrackedDeviceServerDriver* FindTrackedDevice(const char* pchId) override;
    void RunFrame() override;
    bool EnterStandby() override { return true; }
    void LeaveStandby() override {}

private:
    void tracking_thread_func();

    std::unique_ptr<HeadsetDevice> headset_;
    std::unique_ptr<ControllerDevice> left_controller_;
    std::unique_ptr<ControllerDevice> right_controller_;
    std::unique_ptr<InputComponent> input_;

    std::thread tracking_thread_;
    std::atomic<bool> running_{false};

    vr::IDriverLog* log_ = nullptr;
    vr::IDriverServer* server_ = nullptr;
    vr::IDriverHost* host_ = nullptr;

    std::string config_path_;
    std::string install_path_;
};

} // namespace phonevr
