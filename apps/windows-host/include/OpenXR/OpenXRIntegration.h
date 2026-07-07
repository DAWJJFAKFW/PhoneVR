#pragma once

#include <cstdint>
#include <string>
#include <functional>

#ifdef SUPPORT_OPENXR
#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>
#endif

#include "PhoneVRTypes.h"

namespace phonevr {

class OpenXRIntegration {
public:
    OpenXRIntegration() = default;
    ~OpenXRIntegration();

    OpenXRIntegration(const OpenXRIntegration&) = delete;
    OpenXRIntegration& operator=(const OpenXRIntegration&) = delete;

    bool initialize();
    void shutdown();
    bool is_available() const { return available_; }

    bool submit_eye_swapchains(uint64_t left_texture, uint64_t right_texture);
    bool poll_events();
    bool session_running() const { return session_running_; }

    std::function<void(const Pose&)> on_pose_update;

private:
    bool create_instance();
    bool create_session();
    bool create_swapchains();
    void handle_pose_update();

    bool available_ = false;
    bool session_running_ = false;

#ifdef SUPPORT_OPENXR
    XrInstance instance_ = XR_NULL_HANDLE;
    XrSession session_ = XR_NULL_HANDLE;
    XrSystemId system_id_ = XR_NULL_SYSTEM_ID;
    XrSpace local_space_ = XR_NULL_HANDLE;
    XrSpace head_space_ = XR_NULL_HANDLE;

    XrSwapchain left_swapchain_ = XR_NULL_HANDLE;
    XrSwapchain right_swapchain_ = XR_NULL_HANDLE;

    XrViewConfigurationView view_configs_[2] = {};
    uint32_t view_count_ = 0;
#endif

    std::string application_name_ = "PhoneVR";
    std::string engine_name_ = "PhoneVR Engine";
};

} // namespace phonevr
