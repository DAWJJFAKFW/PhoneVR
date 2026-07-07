#pragma once

#include <openvr_driver.h>
#include <string>
#include <atomic>

namespace phonevr {

class HeadsetDevice : public vr::ITrackedDeviceServerDriver,
                      public vr::IVRDisplayComponent,
                      public vr::IVRDriverComponent {
public:
    HeadsetDevice();
    ~HeadsetDevice() override;

    // ITrackedDeviceServerDriver
    vr::EVRInitError Activate(uint32_t unObjectId) override;
    void Deactivate() override;
    void EnterStandby() override {}
    void* GetComponent(const char* pchComponentNameAndVersion) override;
    void DebugRequest(const char* pchRequest, char* pchResponseBuffer,
                      uint32_t unResponseBufferSize) override;
    vr::DriverPose_t GetPose() override;

    // IVRDisplayComponent
    void GetWindowBounds(int32_t* pnX, int32_t* pnY,
                         uint32_t* pnWidth, uint32_t* pnHeight) override;
    bool IsDisplayOnDesktop() override { return false; }
    bool IsDisplayRealDisplay() override { return false; }
    void GetRecommendedRenderTargetSize(uint32_t* pnWidth,
                                         uint32_t* pnHeight) override;
    void GetEyeOutputViewport(EVREye eEye, uint32_t* pnX, uint32_t* pnY,
                              uint32_t* pnWidth, uint32_t* pnHeight) override;
    void GetProjectionRaw(EVREye eEye, float* pfLeft, float* pfRight,
                          float* pfTop, float* pfBottom) override;
    DistortionCoordinates_t ComputeDistortion(EVREye eEye,
                                              float fU, float fV) override;

    // IVRDriverComponent
    void GetTrackedDeviceDriverVersion(CvrPropertyContainer* pContainer) override {}

    // Public methods
    void update_pose(const vr::DriverPose_t& pose);
    uint32_t object_id() const { return object_id_; }
    const std::string& serial_number() const { return serial_number_; }

private:
    uint32_t object_id_ = vr::k_unTrackedDeviceIndexInvalid;
    std::string serial_number_ = "PhoneVR-00001";
    std::string model_number_ = "PhoneVR-1";
    std::string manufacturer_ = "PhoneVR";

    vr::DriverPose_t current_pose_ = {};
    std::atomic<bool> is_active_{false};

    static constexpr float DISPLAY_WIDTH = 0.130f;
    static constexpr float DISPLAY_HEIGHT = 0.075f;
    static constexpr float EYE_TO_SCREEN_DISTANCE = 0.045f;
    static constexpr float LENS_SEPARATION = 0.0635f;
    static constexpr float LENS_CENTER_OFFSET = 0.0f;
};

} // namespace phonevr
