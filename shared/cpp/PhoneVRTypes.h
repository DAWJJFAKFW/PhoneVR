#pragma once

#include <cstdint>
#include <vector>
#include <string>
#include <array>
#include <optional>

namespace phonevr {

// MARK: - Vector & Quaternion

struct alignas(16) Vector3 {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;

    Vector3() = default;
    Vector3(float x, float y, float z) : x(x), y(y), z(z) {}
};

struct alignas(16) Quaternion {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    float w = 1.0f;

    Quaternion() = default;
    Quaternion(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}

    static Quaternion identity() { return Quaternion(0, 0, 0, 1); }
};

// MARK: - Tracking

struct Pose {
    Vector3 position;
    Quaternion orientation;
    uint64_t timestamp_us = 0;
};

struct ImuData {
    Vector3 accelerometer;
    Vector3 gyroscope;
    Vector3 magnetometer;
    uint64_t timestamp_us = 0;
};

enum class HandJointType : uint8_t {
    Wrist = 0,
    ThumbCMC = 1, ThumbMCP = 2, ThumbIP = 3, ThumbTip = 4,
    IndexMCP = 5, IndexPIP = 6, IndexDIP = 7, IndexTip = 8,
    MiddleMCP = 9, MiddlePIP = 10, MiddleDIP = 11, MiddleTip = 12,
    RingMCP = 13, RingPIP = 14, RingDIP = 15, RingTip = 16,
    LittleMCP = 17, LittlePIP = 18, LittleDIP = 19, LittleTip = 20
};

struct HandLandmark {
    Vector3 position;
    float confidence = 0.0f;
    HandJointType joint_type = HandJointType::Wrist;
};

enum class HandGesture : uint8_t {
    Unknown = 0,
    Pinch = 1,
    Grab = 2,
    Point = 3,
    ThumbUp = 4,
    OpenHand = 5,
    Fist = 6
};

struct HandData {
    uint32_t hand_id = 0;
    std::vector<HandLandmark> landmarks;
    float hand_radius = 0.0f;
    Vector3 wrist_position;
    Quaternion wrist_rotation;
    HandGesture gesture = HandGesture::Unknown;
    uint64_t timestamp_us = 0;
};

enum class TrackingQuality : uint8_t {
    Low = 0,
    Medium = 1,
    High = 2
};

struct TrackingFrame {
    Pose head_pose;
    ImuData raw_imu;
    std::optional<HandData> left_hand;
    std::optional<HandData> right_hand;
    uint32_t frame_sequence = 0;
    TrackingQuality quality = TrackingQuality::High;
};

// MARK: - Video

enum class VideoCodec : uint8_t {
    H264 = 0,
    H265 = 1
};

enum class VideoEye : uint8_t {
    Left = 0,
    Right = 1,
    Both = 2
};

struct VideoFrameHeader {
    uint32_t width = 0;
    uint32_t height = 0;
    uint64_t pts_us = 0;
    uint32_t frame_sequence = 0;
    VideoCodec codec = VideoCodec::H264;
    VideoEye eye = VideoEye::Both;
    uint32_t nal_count = 0;
    std::vector<uint8_t> sps_pps;
    uint32_t frame_size = 0;
};

struct VideoPacket {
    VideoFrameHeader header;
    std::vector<uint8_t> nal_data;
    uint32_t packet_sequence = 0;
    uint32_t total_packets = 0;
    bool is_keyframe = false;
};

// MARK: - Control

enum class ControlMessageType : uint8_t {
    Heartbeat = 0,
    Recenter = 1,
    Calibrate = 2,
    SetBitrate = 3,
    SetResolution = 4,
    SetFps = 5,
    SetIpd = 6,
    RequestKeyframe = 7,
    Disconnect = 8,
    StatsReport = 9,
    DeviceInfo = 10
};

struct ControlMessage {
    ControlMessageType type = ControlMessageType::Heartbeat;
    std::vector<uint8_t> payload;
    uint64_t timestamp_us = 0;
    uint32_t message_id = 0;
};

struct StatsReport {
    uint64_t round_trip_time_us = 0;
    uint32_t packets_lost = 0;
    uint32_t packets_sent = 0;
    float bitrate_mbps = 0.0f;
    float encode_time_ms = 0.0f;
    float decode_latency_ms = 0.0f;
    float render_latency_ms = 0.0f;
    float motion_to_photon_latency_ms = 0.0f;
    uint32_t frame_rate = 0;
    uint32_t battery_level = 0;
};

struct DeviceInfo {
    std::string device_name;
    std::string os_version;
    std::string app_version;
    float display_refresh_rate = 0.0f;
    uint32_t display_width = 0;
    uint32_t display_height = 0;
    float display_dpi = 0.0f;
    bool supports_hevc = false;
    uint32_t max_bitrate = 0;
    uint32_t min_bitrate = 0;
    float ipd = 0.0f;
};

// MARK: - Network

struct PacketHeader {
    uint32_t magic = 0x50485256; // PHRV
    uint32_t sequence = 0;
    uint16_t type = 0;
    uint16_t flags = 0;
    uint32_t payload_size = 0;
    uint32_t crc32 = 0;
};

constexpr uint32_t PACKET_HEADER_SIZE = sizeof(PacketHeader);
constexpr uint32_t NETWORK_MAGIC = 0x50485256;
constexpr size_t MAX_PACKET_SIZE = 65536;
constexpr size_t VIDEO_MTU = 1400;

} // namespace phonevr
