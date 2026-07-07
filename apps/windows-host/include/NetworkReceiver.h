#pragma once

#include <cstdint>
#include <vector>
#include <functional>
#include <thread>
#include <atomic>
#include <winsock2.h>
#include <windows.h>

#include "PhoneVRTypes.h"
#include "PhoneVRNetwork.h"

namespace phonevr {

class NetworkReceiver {
public:
    NetworkReceiver();
    ~NetworkReceiver();

    NetworkReceiver(const NetworkReceiver&) = delete;
    NetworkReceiver& operator=(const NetworkReceiver&) = delete;

    bool start(const HostConfig& config);
    void stop();

    std::function<void(const TrackingFrame&)> on_tracking_frame;
    std::function<void(const VideoPacket&)> on_video_packet;
    std::function<void(const HandData&, const HandData&)> on_hand_data;
    std::function<void(const StatsReport&)> on_stats_report;

private:
    void control_thread_func();
    void video_thread_func();
    void process_packet(const uint8_t* data, size_t size);

    std::atomic<bool> running_{false};

    SOCKET control_socket_ = INVALID_SOCKET;
    SOCKET video_socket_ = INVALID_SOCKET;

    std::thread control_thread_;
    std::thread video_thread_;

    uint8_t recv_buffer_[65536];
};

} // namespace phonevr
