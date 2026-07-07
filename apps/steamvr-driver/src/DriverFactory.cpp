#include "DriverFactory.h"
#include "DriverBase.h"
#include <cstring>
#include <memory>

static std::unique_ptr<phonevr::DriverBase> g_driver;

void* HmdDriverFactory(const char* interface_name, int* return_code) {
    if (return_code) {
        *return_code = vr::VRInitError_None;
    }

    if (std::strcmp(interface_name, vr::IDriverBase::Version) == 0) {
        if (!g_driver) {
            g_driver = std::make_unique<phonevr::DriverBase>();
        }
        return g_driver.get();
    }

    if (std::strcmp(interface_name, vr::IServerTrackedDeviceProvider::Version) == 0) {
        if (!g_driver) {
            g_driver = std::make_unique<phonevr::DriverBase>();
        }
        return g_driver.get();
    }

    if (return_code) {
        *return_code = vr::VRInitError_Init_InterfaceNotFound;
    }
    return nullptr;
}
