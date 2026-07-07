#include "Devices/ControllerDevice.h"
#include <cstring>

namespace phonevr {

ControllerDevice::ControllerDevice(const std::string& serial, bool is_left)
    : serial_number_(serial), is_left_(is_left) {
    model_number_ = is_left ? "PhoneVR-Controller-Left" : "PhoneVR-Controller-Right";
}

ControllerDevice::~ControllerDevice() { Deactivate(); }

vr::EVRInitError ControllerDevice::Activate(uint32_t unObjectId) {
    object_id_ = unObjectId;
    is_active_ = true;

    auto props = vr::VRProperties()->TrackedDeviceToPropertyContainer(object_id_);

    vr::VRProperties()->SetStringProperty(props,
        vr::Prop_SerialNumber_String, serial_number_.c_str());
    vr::VRProperties()->SetStringProperty(props,
        vr::Prop_ModelNumber_String, model_number_.c_str());
    vr::VRProperties()->SetStringProperty(props,
        vr::Prop_ManufacturerName_String, "PhoneVR");
    vr::VRProperties()->SetStringProperty(props,
        vr::Prop_RenderModelName_String, is_left_ ?
            "vr_controller_left" : "vr_controller_right");

    vr::VRProperties()->SetInt32Property(props,
        vr::Prop_ControllerRoleHint_Int32,
        is_left_ ? vr::TrackedControllerRole_LeftHand : vr::TrackedControllerRole_RightHand);

    vr::VRProperties()->SetBoolProperty(props,
        vr::Prop_DeviceIsWireless_Bool, true);

    // Set up input components
    vr::VRDriverInput()->CreateSkeletonComponent(
        props,
        is_left_ ? "SkeletonLeft" : "SkeletonRight",
        is_left_ ? "/skeleton/hand/left" : "/skeleton/hand/right",
        "/pose/raw",
        vr::EVRSkeletalTrackingLevel::VRSkeletalTracking_Estimates,
        skeleton_bones_.data(),
        static_cast<int>(skeleton_bones_.size()),
        &skeleton_handle_);

    vr::VRDriverInput()->CreateBooleanComponent(
        props, "/input/grip/click", false, &grip_click_);

    vr::VRDriverInput()->CreateBooleanComponent(
        props, "/input/system/click", false, &system_click_);

    vr::VRDriverInput()->CreateBooleanComponent(
        props, "/input/pinch/click", false, &pinch_click_);

    vr::VRDriverInput()->CreateScalarComponent(
        props, "/input/thumbstick/x", vr::VRScalarType_Absolute,
        vr::VRScalarUnits_NormalizedTwoSided, 0.0f, &thumbstick_handle_);

    vr::VRDriverInput()->CreateScalarComponent(
        props, "/input/thumbstick/y", vr::VRScalarType_Absolute,
        vr::VRScalarUnits_NormalizedTwoSided, 0.0f, &thumbstick_handle_);

    vr::VRDriverInput()->CreateHapticComponent(
        props, "/output/haptic", &haptic_component_);

    vr::VRProperties()->SetStringProperty(props,
        vr::Prop_InputProfilePath_String,
        "{phonevr}/input/phonevr_controller_profile.json");

    return vr::VRInitError_None;
}

void ControllerDevice::Deactivate() {
    is_active_ = false;
    object_id_ = vr::k_unTrackedDeviceIndexInvalid;
}

void* ControllerDevice::GetComponent(const char* pchComponentNameAndVersion) {
    return nullptr;
}

void ControllerDevice::DebugRequest(const char* pchRequest,
                                     char* pchResponseBuffer,
                                     uint32_t unResponseBufferSize) {
    if (unResponseBufferSize > 0) {
        pchResponseBuffer[0] = '\0';
    }
}

vr::DriverPose_t ControllerDevice::GetPose() {
    return current_pose_;
}

void ControllerDevice::update_pose(const vr::DriverPose_t& pose) {
    current_pose_ = pose;
}

void ControllerDevice::update_skeleton(const vr::VRBoneTransform_t* bones,
                                        uint32_t bone_count) {
    if (bone_count > skeleton_bones_.size()) {
        bone_count = static_cast<uint32_t>(skeleton_bones_.size());
    }
    std::memcpy(skeleton_bones_.data(), bones,
                bone_count * sizeof(vr::VRBoneTransform_t));

    if (is_active_ && skeleton_handle_ != vr::k_ulInvalidInputComponentHandle) {
        vr::VRDriverInput()->UpdateSkeletonComponent(
            skeleton_handle_,
            vr::VRSkeletalMotionRange::VRSkeletalMotionRange_WithController,
            skeleton_bones_.data(),
            static_cast<int>(skeleton_bones_.size()));
    }
}

void ControllerDevice::set_gesture(uint32_t gesture_id) {
    if (!is_active_) return;

    // Update boolean components based on gesture
    switch (gesture_id) {
    case 1: // Pinch
        if (pinch_click_ != vr::k_ulInvalidInputComponentHandle) {
            vr::VRDriverInput()->UpdateBooleanComponent(pinch_click_, true, 0);
        }
        if (grip_click_ != vr::k_ulInvalidInputComponentHandle) {
            vr::VRDriverInput()->UpdateBooleanComponent(grip_click_, false, 0);
        }
        break;
    case 2: // Grab
        if (grip_click_ != vr::k_ulInvalidInputComponentHandle) {
            vr::VRDriverInput()->UpdateBooleanComponent(grip_click_, true, 0);
        }
        break;
    default:
        if (pinch_click_ != vr::k_ulInvalidInputComponentHandle) {
            vr::VRDriverInput()->UpdateBooleanComponent(pinch_click_, false, 0);
        }
        if (grip_click_ != vr::k_ulInvalidInputComponentHandle) {
            vr::VRDriverInput()->UpdateBooleanComponent(grip_click_, false, 0);
        }
        break;
    }
}

} // namespace phonevr
