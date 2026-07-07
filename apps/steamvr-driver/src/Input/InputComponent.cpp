#include "Input/InputComponent.h"

namespace phonevr {

bool InputComponent::initialize(vr::IVRDriverInput* driver_input,
                                 vr::PropertyContainerHandle_t container) {
    driver_input_ = driver_input;
    container_ = container;
    return true;
}

void InputComponent::update_gesture(uint32_t gesture_id) {
    if (!driver_input_) return;

    bool grip = (gesture_id == 2); // Grab
    bool pinch = (gesture_id == 1); // Pinch

    if (grip_handle_ != vr::k_ulInvalidInputComponentHandle) {
        driver_input_->UpdateBooleanComponent(grip_handle_, grip, 0);
    }
    if (pinch_handle_ != vr::k_ulInvalidInputComponentHandle) {
        driver_input_->UpdateBooleanComponent(pinch_handle_, pinch, 0);
    }
}

void InputComponent::update_skeleton(const vr::VRBoneTransform_t* bones,
                                      uint32_t count) {
    if (!driver_input_ || skeleton_handle_ == vr::k_ulInvalidInputComponentHandle) {
        return;
    }
    driver_input_->UpdateSkeletonComponent(
        skeleton_handle_,
        vr::VRSkeletalMotionRange::VRSkeletalMotionRange_WithController,
        const_cast<vr::VRBoneTransform_t*>(bones),
        static_cast<int>(count));
}

void InputComponent::update_trackpad(float x, float y) {
    if (!driver_input_ || trackpad_handle_ == vr::k_ulInvalidInputComponentHandle) {
        return;
    }
    // Create a dummy array for the trackpad/touchpad position
    // In production this would use proper scalar components
}

void InputComponent::trigger_haptic() {}

} // namespace phonevr
