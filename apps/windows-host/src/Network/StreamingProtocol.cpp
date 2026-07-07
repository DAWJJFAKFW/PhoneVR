#include "Network/StreamingProtocol.h"
#include <cstring>

namespace phonevr {

std::vector<uint8_t> StreamingProtocol::serialize_tracking_frame(const TrackingFrame& frame) {
    std::vector<uint8_t> data(sizeof(TrackingFrame));
    std::memcpy(data.data(), &frame, sizeof(TrackingFrame));
    return data;
}

TrackingFrame StreamingProtocol::deserialize_tracking_frame(const uint8_t* data, size_t size) {
    TrackingFrame frame = {};
    if (size >= sizeof(TrackingFrame)) {
        std::memcpy(&frame, data, sizeof(TrackingFrame));
    }
    return frame;
}

std::vector<uint8_t> StreamingProtocol::serialize_video_packet(const VideoPacket& packet) {
    size_t header_size = sizeof(VideoPacket);
    size_t total_size = header_size + packet.nal_data.size();

    std::vector<uint8_t> data(total_size);
    std::memcpy(data.data(), &packet, header_size);

    if (!packet.nal_data.empty()) {
        std::memcpy(data.data() + header_size, packet.nal_data.data(), packet.nal_data.size());
    }

    return data;
}

VideoPacket StreamingProtocol::deserialize_video_packet(const uint8_t* data, size_t size) {
    VideoPacket packet = {};
    size_t header_size = sizeof(VideoPacket);

    if (size >= header_size) {
        std::memcpy(&packet, data, header_size);

        if (size > header_size) {
            packet.nal_data.resize(size - header_size);
            std::memcpy(packet.nal_data.data(), data + header_size, size - header_size);
        }
    }

    return packet;
}

std::vector<uint8_t> StreamingProtocol::serialize_control_message(const ControlMessage& msg) {
    size_t header_size = sizeof(ControlMessage);
    size_t total_size = header_size + msg.payload.size();

    std::vector<uint8_t> data(total_size);
    std::memcpy(data.data(), &msg, header_size);

    if (!msg.payload.empty()) {
        std::memcpy(data.data() + header_size, msg.payload.data(), msg.payload.size());
    }

    return data;
}

ControlMessage StreamingProtocol::deserialize_control_message(const uint8_t* data, size_t size) {
    ControlMessage msg = {};
    size_t header_size = sizeof(ControlMessage);

    if (size >= header_size) {
        std::memcpy(&msg, data, header_size);

        if (size > header_size) {
            msg.payload.resize(size - header_size);
            std::memcpy(msg.payload.data(), data + header_size, size - header_size);
        }
    }

    return msg;
}

} // namespace phonevr
