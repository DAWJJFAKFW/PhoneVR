#pragma once

#include <cstdint>
#include <vector>
#include <memory>
#include "PhoneVRTypes.h"

namespace phonevr {

class FrameProcessor {
public:
    FrameProcessor() = default;
    ~FrameProcessor() = default;

    std::vector<uint8_t> process_frame(const uint8_t* data, size_t size,
                                        uint32_t width, uint32_t height,
                                        VideoCodec codec);

    void set_brightness(float brightness) { brightness_ = brightness; }
    void set_contrast(float contrast) { contrast_ = contrast; }

private:
    float brightness_ = 1.0f;
    float contrast_ = 1.0f;
};

} // namespace phonevr
