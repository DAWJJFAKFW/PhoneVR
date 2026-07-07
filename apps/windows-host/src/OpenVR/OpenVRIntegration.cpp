#include "OpenVR/OpenVRIntegration.h"
#include <iostream>

namespace phonevr {

OpenVRIntegration::~OpenVRIntegration() {
    shutdown();
}

bool OpenVRIntegration::initialize() {
#ifdef SUPPORT_STEAMVR
    vr::EVRInitError error = vr::VRInitError_None;
    vr_system_ = vr::VR_Init(&error, vr::VRApplication_Scene);

    if (error != vr::VRInitError_None) {
        std::cerr << "SteamVR init error: " << vr::VR_GetVRInitErrorAsSymbol(error) << std::endl;
        return false;
    }

    vr_compositor_ = vr::VRCompositor();
    if (!vr_compositor_) {
        std::cerr << "Failed to get VR Compositor" << std::endl;
        return false;
    }

    available_ = true;
    return true;
#else
    std::cerr << "SteamVR support not compiled in" << std::endl;
    return false;
#endif
}

void OpenVRIntegration::shutdown() {
#ifdef SUPPORT_STEAMVR
    if (available_) {
        vr::VR_Shutdown();
    }
#endif
    available_ = false;
}

void OpenVRIntegration::submit_poses(const Pose& hmd_pose,
                                      const std::optional<HandData>& left,
                                      const std::optional<HandData>& right) {
#ifdef SUPPORT_STEAMVR
    if (!available_ || !vr_compositor_) return;

    // Wait for poses
    vr_compositor_->WaitGetPoses(poses_, vr::k_unMaxTrackedDeviceCount,
                                  nullptr, 0);

    // Submit to compositor
    vr::Texture_t left_eye_texture = {};
    vr::Texture_t right_eye_texture = {};

    vr::VRTextureBounds_t bounds = {};
    bounds.uMin = 0.0f;
    bounds.uMax = 1.0f;
    bounds.vMin = 0.0f;
    bounds.vMax = 1.0f;

    vr::EVRCompositorError error;

    error = vr_compositor_->Submit(vr::Eye_Left, &left_eye_texture, &bounds,
                                    vr::Submit_Default);
    if (error != vr::VRCompositorError_None) {
        std::cerr << "Submit left eye failed: " << error << std::endl;
    }

    error = vr_compositor_->Submit(vr::Eye_Right, &right_eye_texture, &bounds,
                                    vr::Submit_Default);
    if (error != vr::VRCompositorError_None) {
        std::cerr << "Submit right eye failed: " << error << std::endl;
    }
#endif
}

} // namespace phonevr
