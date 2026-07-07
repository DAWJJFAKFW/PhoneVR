#include "StatsCollector.h"

namespace phonevr {

StatsCollector::StatsCollector()
    : last_bitrate_time_(std::chrono::steady_clock::now()) {}

void StatsCollector::record_frame(uint64_t latency_us) {
    auto now = std::chrono::steady_clock::now();
    std::lock_guard lock(mutex_);

    frame_times_.push_back(
        std::chrono::duration_cast<std::chrono::microseconds>(
            now.time_since_epoch()).count());

    while (!frame_times_.empty() &&
           frame_times_.back() - frame_times_.front() > 1'000'000) {
        frame_times_.erase(frame_times_.begin());
    }

    total_latency_us_ += latency_us;
    frame_count_++;
}

void StatsCollector::record_packet_loss(uint32_t count) {
    packets_lost_ += count;
}

void StatsCollector::record_bitrate(uint64_t bytes, uint64_t duration_us) {
    if (duration_us > 0) {
        double bits = bytes * 8.0;
        double seconds = duration_us / 1'000'000.0;
        current_bitrate_mbps_ = (bits / seconds) / 1'000'000.0;
    }
}

void StatsCollector::record_encode_time(float ms) {
    last_encode_ms_ = ms;
}

void StatsCollector::record_decode_time(float ms) {
    last_decode_ms_ = ms;
}

StatsCollector::Stats StatsCollector::get_stats() const {
    std::lock_guard lock(mutex_);
    Stats s;
    s.fps = static_cast<double>(frame_times_.size());
    s.motion_to_photon_ms = frame_count_ > 0
        ? static_cast<double>(total_latency_us_) / frame_count_ / 1000.0
        : 0.0;
    s.encode_time_ms = last_encode_ms_;
    s.decode_time_ms = last_decode_ms_;
    s.bitrate_mbps = current_bitrate_mbps_;
    s.packets_lost = packets_lost_;
    s.packets_sent = packets_sent_;
    return s;
}

void StatsCollector::reset() {
    std::lock_guard lock(mutex_);
    frame_times_.clear();
    total_latency_us_ = 0;
    frame_count_ = 0;
    current_bitrate_mbps_ = 0.0;
    packets_lost_ = 0;
    packets_sent_ = 0;
}

} // namespace phonevr
