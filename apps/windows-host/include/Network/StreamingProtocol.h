#pragma once

#include <cstdint>
#include <vector>
#include <functional>
#include "PhoneVRTypes.h"

namespace phonevr {

class StreamingProtocol {
public:
    StreamingProtocol() = default;
    ~StreamingProtocol() = default;

    static std::vector<uint8_t> serialize_tracking_frame(const TrackingFrame& frame);
    static TrackingFrame deserialize_tracking_frame(const uint8_t* data, size_t size);

    static std::vector<uint8_t> serialize_video_packet(const VideoPacket& packet);
    static VideoPacket deserialize_video_packet(const uint8_t* data, size_t size);

    static std::vector<uint8_t> serialize_control_message(const ControlMessage& msg);
    static ControlMessage deserialize_control_message(const uint8_t* data, size_t size);
};

} // namespace phonevr
