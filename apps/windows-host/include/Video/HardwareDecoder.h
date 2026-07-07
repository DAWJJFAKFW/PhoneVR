#pragma once

#include <cstdint>
#include <vector>
#include <memory>
#include <optional>
#include <wrl/client.h>
#include <mfapi.h>
#include <mftransform.h>
#include <mfidl.h>

#include "PhoneVRTypes.h"

using Microsoft::WRL::ComPtr;

namespace phonevr {

struct DecodedFrame {
    std::vector<uint8_t> data;
    uint32_t width;
    uint32_t height;
    VideoCodec codec;
    VideoEye eye;
    uint64_t pts_us;
    bool is_keyframe;
};

class HardwareDecoder {
public:
    HardwareDecoder();
    ~HardwareDecoder();

    HardwareDecoder(const HardwareDecoder&) = delete;
    HardwareDecoder& operator=(const HardwareDecoder&) = delete;

    bool initialize();
    void shutdown();

    std::optional<DecodedFrame> decode(const VideoPacket& packet);

private:
    bool create_decoder(VideoCodec codec, uint32_t width, uint32_t height,
                        const std::vector<uint8_t>& sps_pps);
    bool initialized_ = false;

    ComPtr<IMFTransform> decoder_;
    ComPtr<IMFMediaBuffer> output_buffer_;
    ComPtr<IMFSample> output_sample_;

    VideoCodec current_codec_ = VideoCodec::H264;
    uint32_t current_width_ = 0;
    uint32_t current_height_ = 0;
};

} // namespace phonevr
