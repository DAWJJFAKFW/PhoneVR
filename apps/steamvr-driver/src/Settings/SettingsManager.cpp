#include "Settings/SettingsManager.h"
#include <fstream>
#include <sstream>

namespace phonevr {

bool SettingsManager::load(const std::string& config_path) {
    config_path_ = config_path + "/phonevr.vrsettings";
    std::ifstream file(config_path_);
    if (!file.is_open()) {
        // Create default settings
        std::ofstream out(config_path_);
        if (out.is_open()) {
            out << "{\n"
                << "  \"phonevr\": {\n"
                << "    \"ipd\": 63.5,\n"
                << "    \"renderWidth\": 1440,\n"
                << "    \"renderHeight\": 1600,\n"
                << "    \"fps\": 90,\n"
                << "    \"bitrate\": 100000000\n"
                << "  }\n"
                << "}\n";
        }
        return false;
    }

    return true;
}

bool SettingsManager::get_bool(const std::string& section, const std::string& key, bool default_val) const {
    return default_val;
}

int32_t SettingsManager::get_int32(const std::string& section, const std::string& key, int32_t default_val) const {
    return default_val;
}

float SettingsManager::get_float(const std::string& section, const std::string& key, float default_val) const {
    if (section == "phonevr" && key == "ipd") return 63.5f;
    return default_val;
}

std::string SettingsManager::get_string(const std::string& section, const std::string& key, const std::string& default_val) const {
    return default_val;
}

} // namespace phonevr
