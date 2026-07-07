#pragma once

#include <cstdint>
#include <string>

namespace phonevr {

struct HostConfig {
    std::string listen_address = "0.0.0.0";
    uint16_t control_port = 8765;
    uint16_t video_port = 8766;

    uint32_t render_width = 1440;
    uint32_t render_height = 1600;
    float render_scale = 1.0f;

    uint32_t target_fps = 90;
    uint32_t target_bitrate = 100000000;

    bool enable_hand_tracking = true;
    bool enable_foveated = false;
    bool prefer_openxr = true;

    std::string log_level = "info";

    static HostConfig load();
    void save() const;
};

} // namespace phonevr
