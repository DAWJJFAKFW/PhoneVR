#include <iostream>
#include <csignal>
#include <windows.h>

#include "Config.h"
#include "NetworkReceiver.h"
#include "StatsCollector.h"
#include "Video/HardwareDecoder.h"
#include "Render/D3D12Renderer.h"

std::atomic<bool> g_running = true;

void signal_handler(int) {
    g_running = false;
}

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    auto config = phonevr::HostConfig::load();
    phonevr::NetworkReceiver receiver;
    phonevr::StatsCollector stats;
    phonevr::HardwareDecoder decoder;
    phonevr::D3D12Renderer renderer;

    std::cout << "PhoneVR Host v1.0.0" << std::endl;
    std::cout << "Listening on " << config.listen_address
              << ":" << config.control_port << std::endl;

    receiver.on_tracking_frame = [&](const phonevr::TrackingFrame& frame) {
        renderer.update_pose(frame.head_pose);
        if (frame.left_hand || frame.right_hand) {
            renderer.update_hands(frame.left_hand, frame.right_hand);
        }
    };

    receiver.on_video_packet = [&](const phonevr::VideoPacket& packet) {
        auto frame = decoder.decode(packet);
        if (frame) {
            renderer.present_frame(*frame, packet.header.eye);
            stats.record_frame(packet.header.pts_us);
        }
    };

    receiver.on_stats_report = [&](const phonevr::StatsReport& report) {
        stats.record_packet_loss(report.packets_lost);
    };

    if (!renderer.initialize(config)) {
        std::cerr << "Failed to initialize renderer" << std::endl;
        return 1;
    }

    if (!receiver.start(config)) {
        std::cerr << "Failed to start network receiver" << std::endl;
        return 1;
    }

    MSG msg = {};
    while (g_running && msg.message != WM_QUIT) {
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        renderer.render_frame();
    }

    receiver.stop();
    return 0;
}
