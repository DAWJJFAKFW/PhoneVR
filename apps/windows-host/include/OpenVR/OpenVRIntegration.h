#pragma once

#include <cstdint>
#include <string>
#include <functional>

#ifdef SUPPORT_STEAMVR
#include <openvr/openvr.h>
#endif

#include "PhoneVRTypes.h"

namespace phonevr {

class OpenVRIntegration {
public:
    OpenVRIntegration() = default;
    ~OpenVRIntegration();

    OpenVRIntegration(const OpenVRIntegration&) = delete;
    OpenVRIntegration& operator=(const OpenVRIntegration&) = delete;

    bool initialize();
    void shutdown();
    bool is_available() const { return available_; }

    void submit_poses(const Pose& hmd_pose,
                      const std::optional<HandData>& left,
                      const std::optional<HandData>& right);

private:
    bool available_ = false;

#ifdef SUPPORT_STEAMVR
    vr::IVRSystem* vr_system_ = nullptr;
    vr::IVRCompositor* vr_compositor_ = nullptr;
    vr::TrackedDevicePose_t poses_[vr::k_unMaxTrackedDeviceCount];
#endif
};

} // namespace phonevr
