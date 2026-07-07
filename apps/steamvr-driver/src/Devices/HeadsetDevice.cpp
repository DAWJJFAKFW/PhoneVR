#include "Devices/HeadsetDevice.h"
#include <cstring>
#include <cmath>

namespace phonevr {

HeadsetDevice::HeadsetDevice() = default;
HeadsetDevice::~HeadsetDevice() { Deactivate(); }

vr::EVRInitError HeadsetDevice::Activate(uint32_t unObjectId) {
    object_id_ = unObjectId;
    is_active_ = true;

    // Set up display properties
    vr::VRProperties()->SetStringProperty(
        vr::VRProperties()->TrackedDeviceToPropertyContainer(object_id_),
        vr::Prop_TrackingSystemName_String, "PhoneVR");
    vr::VRProperties()->SetStringProperty(
        vr::VRProperties()->TrackedDeviceToPropertyContainer(object_id_),
        vr::Prop_SerialNumber_String, serial_number_.c_str());
    vr::VRProperties()->SetStringProperty(
        vr::VRProperties()->TrackedDeviceToPropertyContainer(object_id_),
        vr::Prop_ModelNumber_String, model_number_.c_str());
    vr::VRProperties()->SetStringProperty(
        vr::VRProperties()->TrackedDeviceToPropertyContainer(object_id_),
        vr::Prop_ManufacturerName_String, manufacturer_.c_str());

    vr::VRProperties()->SetBoolProperty(
        vr::VRProperties()->TrackedDeviceToPropertyContainer(object_id_),
        vr::Prop_WillDriftInYaw_Bool, false);
    vr::VRProperties()->SetBoolProperty(
        vr::VRProperties()->TrackedDeviceToPropertyContainer(object_id_),
        vr::Prop_DeviceIsWireless_Bool, true);
    vr::VRProperties()->SetBoolProperty(
        vr::VRProperties()->TrackedDeviceToPropertyContainer(object_id_),
        vr::Prop_DeviceProvidesChaperone_Bool, false);

    // Display properties
    vr::VRProperties()->SetFloatProperty(
        vr::VRProperties()->TrackedDeviceToPropertyContainer(object_id_),
        vr::Prop_DisplayFrequency_Float, 90.0f);
    vr::VRProperties()->SetFloatProperty(
        vr::VRProperties()->TrackedDeviceToPropertyContainer(object_id_),
        vr::Prop_SecondsFromVsyncToPhotons_Float, 0.011f);
    vr::VRProperties()->SetFloatProperty(
        vr::VRProperties()->TrackedDeviceToPropertyContainer(object_id_),
        vr::Prop_DisplayMCAlpha_Float, 0.0f);
    vr::VRProperties()->SetFloatProperty(
        vr::VRProperties()->TrackedDeviceToPropertyContainer(object_id_),
        vr::Prop_DisplayMCDeltaX_Float, 0.0f);
    vr::VRProperties()->SetFloatProperty(
        vr::VRProperties()->TrackedDeviceToPropertyContainer(object_id_),
        vr::Prop_DisplayMCDeltaY_Float, 0.0f);
    vr::VRProperties()->SetFloatProperty(
        vr::VRProperties()->TrackedDeviceToPropertyContainer(object_id_),
        vr::Prop_DisplayMCDeltaZ_Float, 0.0f);

    return vr::VRInitError_None;
}

void HeadsetDevice::Deactivate() {
    is_active_ = false;
    object_id_ = vr::k_unTrackedDeviceIndexInvalid;
}

void* HeadsetDevice::GetComponent(const char* pchComponentNameAndVersion) {
    if (std::strcmp(pchComponentNameAndVersion,
                    vr::IVRDisplayComponent::Version) == 0) {
        return static_cast<vr::IVRDisplayComponent*>(this);
    }
    if (std::strcmp(pchComponentNameAndVersion,
                    vr::IVRDriverComponent::Version) == 0) {
        return static_cast<vr::IVRDriverComponent*>(this);
    }
    return nullptr;
}

void HeadsetDevice::DebugRequest(const char* pchRequest, char* pchResponseBuffer,
                                  uint32_t unResponseBufferSize) {
    if (unResponseBufferSize > 0) {
        pchResponseBuffer[0] = '\0';
    }
}

vr::DriverPose_t HeadsetDevice::GetPose() {
    return current_pose_;
}

void HeadsetDevice::GetWindowBounds(int32_t* pnX, int32_t* pnY,
                                     uint32_t* pnWidth, uint32_t* pnHeight) {
    *pnX = 0;
    *pnY = 0;
    *pnWidth = 2880;
    *pnHeight = 1600;
}

void HeadsetDevice::GetRecommendedRenderTargetSize(uint32_t* pnWidth,
                                                    uint32_t* pnHeight) {
    *pnWidth = 1440;
    *pnHeight = 1600;
}

void HeadsetDevice::GetEyeOutputViewport(EVREye eEye, uint32_t* pnX,
                                          uint32_t* pnY, uint32_t* pnWidth,
                                          uint32_t* pnHeight) {
    *pnY = 0;
    *pnHeight = 1600;

    if (eEye == Eye_Left) {
        *pnX = 0;
        *pnWidth = 1440;
    } else {
        *pnX = 1440;
        *pnWidth = 1440;
    }
}

void HeadsetDevice::GetProjectionRaw(EVREye eEye, float* pfLeft,
                                      float* pfRight, float* pfTop,
                                      float* pfBottom) {
    float half_display = DISPLAY_WIDTH * 0.5f;
    float center_offset = LENS_CENTER_OFFSET;

    if (eEye == Eye_Left) {
        *pfLeft = -half_display + center_offset / EYE_TO_SCREEN_DISTANCE;
        *pfRight = half_display + center_offset / EYE_TO_SCREEN_DISTANCE;
    } else {
        *pfLeft = -half_display - center_offset / EYE_TO_SCREEN_DISTANCE;
        *pfRight = half_display - center_offset / EYE_TO_SCREEN_DISTANCE;
    }

    float half_height = DISPLAY_HEIGHT * 0.5f;
    *pfTop = -half_height;
    *pfBottom = half_height;
}

vr::DistortionCoordinates_t HeadsetDevice::ComputeDistortion(
    EVREye eEye, float fU, float fV)
{
    vr::DistortionCoordinates_t coords;

    float u = (fU * 2.0f - 1.0f);
    float v = (fV * 2.0f - 1.0f);

    float r2 = u * u + v * v;
    float r4 = r2 * r2;
    float dist = 1.0f + (-0.15f) * r2 + 0.05f * r4;

    coords.rfBlue[0] = (u * dist + 1.0f) * 0.5f;
    coords.rfBlue[1] = (v * dist + 1.0f) * 0.5f;
    coords.rfGreen[0] = (u * dist + 1.0f) * 0.5f;
    coords.rfGreen[1] = (v * dist + 1.0f) * 0.5f;
    coords.rfRed[0] = (u * dist + 1.0f) * 0.5f;
    coords.rfRed[1] = (v * dist + 1.0f) * 0.5f;

    return coords;
}

void HeadsetDevice::update_pose(const vr::DriverPose_t& pose) {
    current_pose_ = pose;
}

} // namespace phonevr
