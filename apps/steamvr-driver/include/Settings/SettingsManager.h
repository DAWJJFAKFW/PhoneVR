#pragma once

#include <string>
#include <cstdint>

namespace phonevr {

class SettingsManager {
public:
    SettingsManager() = default;
    ~SettingsManager() = default;

    bool load(const std::string& config_path);

    bool get_bool(const std::string& section, const std::string& key, bool default_val = false) const;
    int32_t get_int32(const std::string& section, const std::string& key, int32_t default_val = 0) const;
    float get_float(const std::string& section, const std::string& key, float default_val = 0.0f) const;
    std::string get_string(const std::string& section, const std::string& key, const std::string& default_val = "") const;

private:
    std::string config_path_;
};

} // namespace phonevr
