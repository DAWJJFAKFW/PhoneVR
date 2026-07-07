#include "Video/FrameProcessor.h"
#include <algorithm>
#include <cstring>

namespace phonevr {

std::vector<uint8_t> FrameProcessor::process_frame(const uint8_t* data, size_t size,
                                                     uint32_t width, uint32_t height,
                                                     VideoCodec codec) {
    if (!data || size == 0) return {};

    // NV12 format processing (brightness/contrast adjustment)
    if (codec == VideoCodec::H264 || codec == VideoCodec::H265) {
        size_t y_size = width * height;
        size_t uv_size = y_size / 2;

        if (size < y_size + uv_size) {
            return std::vector<uint8_t>(data, data + size);
        }

        std::vector<uint8_t> processed(data, data + size);

        // Adjust Y plane (luma)
        uint8_t* y_plane = processed.data();
        for (size_t i = 0; i < y_size; ++i) {
            float value = static_cast<float>(y_plane[i]) * brightness_;
            value = (value - 128.0f) * contrast_ + 128.0f;
            y_plane[i] = static_cast<uint8_t>(std::clamp(value, 0.0f, 255.0f));
        }

        return processed;
    }

    return std::vector<uint8_t>(data, data + size);
}

} // namespace phonevr
