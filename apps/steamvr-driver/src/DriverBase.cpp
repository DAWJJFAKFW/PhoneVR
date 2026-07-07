#include "DriverBase.h"
#include <fstream>
#include <chrono>

namespace phonevr {

DriverBase::DriverBase() = default;
DriverBase::~DriverBase() { Cleanup(); }

vr::EVRInitError DriverBase::Init(
    vr::IDriverLog* pDriverLog,
    vr::IDriverServer* pDriverServer,
    vr::IDriverHost* pDriverHost,
    const char* pchUserDriverConfigDir,
    const char* pchDriverInstallDir)
{
    log_ = pDriverLog;
    server_ = pDriverServer;
    host_ = pDriverHost;
    config_path_ = pchUserDriverConfigDir;
    install_path_ = pchDriverInstallDir;

    log_->Log("PhoneVR Driver initializing...");

    headset_ = std::make_unique<HeadsetDevice>();
    left_controller_ = std::make_unique<ControllerDevice>("PhoneVR-Left-00001", true);
    right_controller_ = std::make_unique<ControllerDevice>("PhoneVR-Right-00001", false);

    vr::EVRInitError err = vr::VRInitError_None;

    if (!server_->TrackedDeviceAdded(
            headset_->serial_number().c_str(),
            vr::TrackedDeviceClass_HMD, headset_.get())) {
        log_->Log("Failed to add HMD device");
        return vr::VRInitError_Driver_Failed;
    }

    if (!server_->TrackedDeviceAdded(
            left_controller_->serial_number().c_str(),
            vr::TrackedDeviceClass_Controller, left_controller_.get())) {
        log_->Log("Failed to add left controller");
    }

    if (!server_->TrackedDeviceAdded(
            right_controller_->serial_number().c_str(),
            vr::TrackedDeviceClass_Controller, right_controller_.get())) {
        log_->Log("Failed to add right controller");
    }

    running_ = true;
    tracking_thread_ = std::thread(&DriverBase::tracking_thread_func, this);

    log_->Log("PhoneVR Driver initialized successfully");
    return vr::VRInitError_None;
}

void DriverBase::Cleanup() {
    running_ = false;
    if (tracking_thread_.joinable()) {
        tracking_thread_.join();
    }

    headset_.reset();
    left_controller_.reset();
    right_controller_.reset();
}

uint32_t DriverBase::GetTrackedDeviceCount() {
    return 3;
}

vr::ITrackedDeviceServerDriver* DriverBase::GetTrackedDevice(uint32_t unWhich) {
    switch (unWhich) {
    case 0: return headset_.get();
    case 1: return left_controller_.get();
    case 2: return right_controller_.get();
    default: return nullptr;
    }
}

vr::ITrackedDeviceServerDriver* DriverBase::FindTrackedDevice(const char* pchId) {
    if (headset_ && headset_->serial_number() == pchId) return headset_.get();
    if (left_controller_ && left_controller_->serial_number() == pchId) return left_controller_.get();
    if (right_controller_ && right_controller_->serial_number() == pchId) return right_controller_.get();
    return nullptr;
}

void DriverBase::RunFrame() {
    if (headset_) {
        host_->TrackedDevicePoseUpdated(
            headset_->object_id(), headset_->GetPose(), sizeof(vr::DriverPose_t));
    }
    if (left_controller_) {
        host_->TrackedDevicePoseUpdated(
            left_controller_->object_id(), left_controller_->GetPose(), sizeof(vr::DriverPose_t));
    }
    if (right_controller_) {
        host_->TrackedDevicePoseUpdated(
            right_controller_->object_id(), right_controller_->GetPose(), sizeof(vr::DriverPose_t));
    }
}

void DriverBase::tracking_thread_func() {
    while (running_) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));

        auto now = std::chrono::steady_clock::now();
        auto us = std::chrono::duration_cast<std::chrono::microseconds>(
            now.time_since_epoch()).count();

        // Update poses with current tracking data
        // In production, this reads from the shared memory/pipe from Windows Host

        // Headset pose
        vr::DriverPose_t hmd_pose = {};
        hmd_pose.poseTimeOffset = 0;
        hmd_pose.qWorldFromDriverRotation = {1, 0, 0, 0};
        hmd_pose.qDriverFromHeadRotation = {1, 0, 0, 0};
        hmd_pose.qRotation = {1, 0, 0, 0};
        hmd_pose.vecPosition = {0, 1.5f, 0};
        hmd_pose.poseIsValid = true;
        hmd_pose.result = vr::TrackingResult_Running_OK;
        hmd_pose.deviceIsConnected = true;
        hmd_pose.willDriftInYaw = false;
        hmd_pose.shouldApplyHeadModel = false;

        headset_->update_pose(hmd_pose);

        // Left controller pose
        vr::DriverPose_t left_pose = {};
        left_pose.poseTimeOffset = 0;
        left_pose.qWorldFromDriverRotation = {1, 0, 0, 0};
        left_pose.qDriverFromHeadRotation = {1, 0, 0, 0};
        left_pose.qRotation = {1, 0, 0, 0};
        left_pose.vecPosition = {-0.3f, 1.2f, -0.2f};
        left_pose.poseIsValid = true;
        left_pose.result = vr::TrackingResult_Running_OK;
        left_pose.deviceIsConnected = true;
        left_pose.willDriftInYaw = false;
        left_pose.shouldApplyHeadModel = true;

        left_controller_->update_pose(left_pose);

        // Right controller pose
        vr::DriverPose_t right_pose = {};
        right_pose.poseTimeOffset = 0;
        right_pose.qWorldFromDriverRotation = {1, 0, 0, 0};
        right_pose.qDriverFromHeadRotation = {1, 0, 0, 0};
        right_pose.qRotation = {1, 0, 0, 0};
        right_pose.vecPosition = {0.3f, 1.2f, -0.2f};
        right_pose.poseIsValid = true;
        right_pose.result = vr::TrackingResult_Running_OK;
        right_pose.deviceIsConnected = true;
        right_pose.willDriftInYaw = false;
        right_pose.shouldApplyHeadModel = true;

        right_controller_->update_pose(right_pose);
    }
}

} // namespace phonevr
