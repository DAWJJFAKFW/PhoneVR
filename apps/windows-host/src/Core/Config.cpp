#include "Config.h"
#include <fstream>
#include <filesystem>
#include <nlohmann/json.hpp>

namespace phonevr {

using json = nlohmann::json;

HostConfig HostConfig::load() {
    HostConfig config;
    std::string path = std::filesystem::current_path().string() + "/phonevr_config.json";

    std::ifstream file(path);
    if (!file.is_open()) return config;

    try {
        json j;
        file >> j;

        config.listen_address = j.value("listen_address", config.listen_address);
        config.control_port = j.value("control_port", config.control_port);
        config.video_port = j.value("video_port", config.video_port);
        config.render_width = j.value("render_width", config.render_width);
        config.render_height = j.value("render_height", config.render_height);
        config.render_scale = j.value("render_scale", config.render_scale);
        config.target_fps = j.value("target_fps", config.target_fps);
        config.target_bitrate = j.value("target_bitrate", config.target_bitrate);
        config.enable_hand_tracking = j.value("enable_hand_tracking", config.enable_hand_tracking);
        config.enable_foveated = j.value("enable_foveated", config.enable_foveated);
        config.prefer_openxr = j.value("prefer_openxr", config.prefer_openxr);
        config.log_level = j.value("log_level", config.log_level);
    } catch (...) {}

    return config;
}

void HostConfig::save() const {
    std::string path = std::filesystem::current_path().string() + "/phonevr_config.json";

    json j;
    j["listen_address"] = listen_address;
    j["control_port"] = control_port;
    j["video_port"] = video_port;
    j["render_width"] = render_width;
    j["render_height"] = render_height;
    j["render_scale"] = render_scale;
    j["target_fps"] = target_fps;
    j["target_bitrate"] = target_bitrate;
    j["enable_hand_tracking"] = enable_hand_tracking;
    j["enable_foveated"] = enable_foveated;
    j["prefer_openxr"] = prefer_openxr;
    j["log_level"] = log_level;

    std::ofstream file(path);
    if (file.is_open()) {
        file << j.dump(4);
    }
}

} // namespace phonevr
