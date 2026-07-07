#include <cassert>
#include <iostream>
#include <thread>
#include <chrono>
#include "../../shared/cpp/PhoneVRNetwork.h"
#include "../../shared/cpp/PhoneVRTypes.h"

void test_tracking_frame_serialization() {
    phonevr::TrackingFrame frame;
    frame.frame_sequence = 1;
    frame.quality = phonevr::TrackingQuality::High;
    frame.head_pose.position = phonevr::Vector3(0, 1.5f, 0);
    frame.head_pose.orientation = phonevr::Quaternion(0, 0, 0, 1);

    const uint8_t* data = reinterpret_cast<const uint8_t*>(&frame);
    size_t size = sizeof(phonevr::TrackingFrame);

    phonevr::TrackingFrame parsed;
    std::memcpy(&parsed, data, size);

    assert(parsed.frame_sequence == 1);
    assert(parsed.quality == phonevr::TrackingQuality::High);
    assert(parsed.head_pose.position.y == 1.5f);

    std::cout << "Tracking frame serialization test passed" << std::endl;
}

void test_video_packet() {
    phonevr::VideoPacket packet;
    packet.header.width = 1440;
    packet.header.height = 1600;
    packet.header.codec = phonevr::VideoCodec::H265;
    packet.header.eye = phonevr::VideoEye::Left;
    packet.is_keyframe = true;
    packet.nal_data = {0x00, 0x00, 0x00, 0x01, 0x67, 0x42, 0x00, 0x1e};

    auto packet_data = phonevr::build_packet(
        phonevr::PacketType::Video,
        reinterpret_cast<const uint8_t*>(&packet),
        sizeof(packet), 1,
        phonevr::PacketFlag_Keyframe);

    auto result = phonevr::parse_packet(packet_data.data(), packet_data.size());
    assert(result.has_value());

    phonevr::VideoPacket parsed;
    std::memcpy(&parsed, result->second.data(), sizeof(phonevr::VideoPacket));

    assert(parsed.header.width == 1440);
    assert(parsed.header.height == 1600);
    assert(parsed.header.codec == phonevr::VideoCodec::H265);
    assert(parsed.is_keyframe == true);

    std::cout << "Video packet test passed" << std::endl;
}

int main() {
    test_tracking_frame_serialization();
    test_video_packet();

    std::cout << "All Windows tests passed!" << std::endl;
    return 0;
}
