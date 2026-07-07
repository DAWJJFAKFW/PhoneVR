#pragma once

#include <openvr_driver.h>
#include <string>
#include <atomic>
#include <array>

namespace phonevr {

class ControllerDevice : public vr::ITrackedDeviceServerDriver {
public:
    ControllerDevice(const std::string& serial, bool is_left);
    ~ControllerDevice() override;

    // ITrackedDeviceServerDriver
    vr::EVRInitError Activate(uint32_t unObjectId) override;
    void Deactivate() override;
    void EnterStandby() override {}
    void* GetComponent(const char* pchComponentNameAndVersion) override;
    void DebugRequest(const char* pchRequest, char* pchResponseBuffer,
                      uint32_t unResponseBufferSize) override;
    vr::DriverPose_t GetPose() override;

    // Public methods
    void update_pose(const vr::DriverPose_t& pose);
    void update_skeleton(const vr::VRBoneTransform_t* bones, uint32_t bone_count);
    void set_gesture(uint32_t gesture_id);

    uint32_t object_id() const { return object_id_; }
    const std::string& serial_number() const { return serial_number_; }
    bool is_left() const { return is_left_; }

    // Input components
    vr::VRInputComponentHandle_t skeleton_handle_ = vr::k_ulInvalidInputComponentHandle;
    vr::VRInputComponentHandle_t grip_click_ = vr::k_ulInvalidInputComponentHandle;
    vr::VRInputComponentHandle_t pinch_click_ = vr::k_ulInvalidInputComponentHandle;
    vr::VRInputComponentHandle_t system_click_ = vr::k_ulInvalidInputComponentHandle;
    vr::VRInputComponentHandle_t thumbstick_handle_ = vr::k_ulInvalidInputComponentHandle;
    vr::VRInputComponentHandle_t trackpad_handle_ = vr::k_ulInvalidInputComponentHandle;
    vr::VRInputComponentHandle_t haptic_component_ = vr::k_ulInvalidInputComponentHandle;

private:
    uint32_t object_id_ = vr::k_unTrackedDeviceIndexInvalid;
    std::string serial_number_;
    std::string model_number_;
    bool is_left_;

    vr::DriverPose_t current_pose_ = {};
    std::atomic<bool> is_active_{false};

    std::array<vr::VRBoneTransform_t, 31> skeleton_bones_;
    uint32_t hand_skeleton_level_ = 3;
};

} // namespace phonevr
