#pragma once

#include <openvr_driver.h>
#include <cstdint>

namespace phonevr {

class InputComponent {
public:
    InputComponent() = default;
    ~InputComponent() = default;

    bool initialize(vr::IVRDriverInput* driver_input,
                    vr::PropertyContainerHandle_t container);

    void update_gesture(uint32_t gesture_id);
    void update_skeleton(const vr::VRBoneTransform_t* bones, uint32_t count);
    void update_trackpad(float x, float y);
    void trigger_haptic();

private:
    vr::IVRDriverInput* driver_input_ = nullptr;
    vr::PropertyContainerHandle_t container_ = vr::k_ulInvalidPropertyContainer;

    vr::VRInputComponentHandle_t skeleton_handle_ = vr::k_ulInvalidInputComponentHandle;
    vr::VRInputComponentHandle_t grip_handle_ = vr::k_ulInvalidInputComponentHandle;
    vr::VRInputComponentHandle_t pinch_handle_ = vr::k_ulInvalidInputComponentHandle;
    vr::VRInputComponentHandle_t trackpad_handle_ = vr::k_ulInvalidInputComponentHandle;
};

} // namespace phonevr
