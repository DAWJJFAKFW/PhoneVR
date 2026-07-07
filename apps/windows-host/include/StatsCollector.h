#pragma once

#include <cstdint>
#include <atomic>
#include <chrono>
#include <vector>
#include <mutex>

namespace phonevr {

class StatsCollector {
public:
    StatsCollector();

    void record_frame(uint64_t latency_us);
    void record_packet_loss(uint32_t count);
    void record_bitrate(uint64_t bytes, uint64_t duration_us);
    void record_encode_time(float ms);
    void record_decode_time(float ms);

    struct Stats {
        double fps = 0.0;
        double motion_to_photon_ms = 0.0;
        double encode_time_ms = 0.0;
        double decode_time_ms = 0.0;
        double bitrate_mbps = 0.0;
        uint32_t packets_lost = 0;
        uint32_t packets_sent = 0;
        uint32_t battery_level = 0;
    };

    Stats get_stats() const;
    void reset();

private:
    mutable std::mutex mutex_;

    std::vector<uint64_t> frame_times_;
    std::atomic<uint64_t> total_latency_us_{0};
    std::atomic<uint64_t> frame_count_{0};
    std::atomic<double> current_bitrate_mbps_{0.0};
    std::atomic<float> last_encode_ms_{0.0f};
    std::atomic<float> last_decode_ms_{0.0f};
    std::atomic<uint32_t> packets_lost_{0};
    std::atomic<uint32_t> packets_sent_{0};

    std::chrono::steady_clock::time_point last_bitrate_time_;
    uint64_t last_bitrate_bytes_ = 0;
};

} // namespace phonevr
