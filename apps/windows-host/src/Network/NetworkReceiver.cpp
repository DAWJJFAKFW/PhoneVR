#include "NetworkReceiver.h"
#include <iostream>

namespace phonevr {

NetworkReceiver::NetworkReceiver() {
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);
}

NetworkReceiver::~NetworkReceiver() {
    stop();
    WSACleanup();
}

bool NetworkReceiver::start(const HostConfig& config) {
    running_ = true;

    control_socket_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (control_socket_ == INVALID_SOCKET) {
        std::cerr << "Failed to create control socket" << std::endl;
        return false;
    }

    int opt = 1;
    setsockopt(control_socket_, SOL_SOCKET, SO_REUSEADDR,
               reinterpret_cast<const char*>(&opt), sizeof(opt));

    sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(config.control_port);

    if (bind(control_socket_, reinterpret_cast<const sockaddr*>(&addr),
             sizeof(addr)) == SOCKET_ERROR) {
        std::cerr << "Failed to bind control socket: " << WSAGetLastError() << std::endl;
        return false;
    }

    listen(control_socket_, 5);

    video_socket_ = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (video_socket_ == INVALID_SOCKET) {
        std::cerr << "Failed to create video socket" << std::endl;
        return false;
    }

    addr.sin_port = htons(config.video_port);
    if (bind(video_socket_, reinterpret_cast<const sockaddr*>(&addr),
             sizeof(addr)) == SOCKET_ERROR) {
        std::cerr << "Failed to bind video socket: " << WSAGetLastError() << std::endl;
        return false;
    }

    DWORD timeout = 100;
    setsockopt(video_socket_, SOL_SOCKET, SO_RCVTIMEO,
               reinterpret_cast<const char*>(&timeout), sizeof(timeout));

    control_thread_ = std::thread(&NetworkReceiver::control_thread_func, this);
    video_thread_ = std::thread(&NetworkReceiver::video_thread_func, this);

    return true;
}

void NetworkReceiver::stop() {
    running_ = false;

    if (control_socket_ != INVALID_SOCKET) {
        closesocket(control_socket_);
        control_socket_ = INVALID_SOCKET;
    }
    if (video_socket_ != INVALID_SOCKET) {
        closesocket(video_socket_);
        video_socket_ = INVALID_SOCKET;
    }

    if (control_thread_.joinable()) control_thread_.join();
    if (video_thread_.joinable()) video_thread_.join();
}

void NetworkReceiver::control_thread_func() {
    while (running_) {
        fd_set read_set;
        FD_ZERO(&read_set);
        FD_SET(control_socket_, &read_set);

        timeval tv = {0, 100000};
        int result = select(0, &read_set, nullptr, nullptr, &tv);

        if (result > 0 && FD_ISSET(control_socket_, &read_set)) {
            sockaddr_in client_addr;
            int addr_len = sizeof(client_addr);
            SOCKET client = accept(control_socket_,
                                   reinterpret_cast<sockaddr*>(&client_addr),
                                   &addr_len);

            if (client != INVALID_SOCKET) {
                char client_ip[64] = {};
                inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, sizeof(client_ip));
                std::cout << "Client connected from " << client_ip << std::endl;

                while (running_) {
                    int bytes = recv(client, reinterpret_cast<char*>(recv_buffer_),
                                     sizeof(recv_buffer_), 0);
                    if (bytes <= 0) break;

                    process_packet(recv_buffer_, bytes);
                }

                closesocket(client);
                std::cout << "Client disconnected" << std::endl;
            }
        }
    }
}

void NetworkReceiver::video_thread_func() {
    while (running_) {
        sockaddr_in from;
        int from_len = sizeof(from);

        int bytes = recvfrom(video_socket_,
                             reinterpret_cast<char*>(recv_buffer_),
                             sizeof(recv_buffer_), 0,
                             reinterpret_cast<sockaddr*>(&from),
                             &from_len);

        if (bytes > 0) {
            process_packet(recv_buffer_, bytes);
        }
    }
}

void NetworkReceiver::process_packet(const uint8_t* data, size_t size) {
    auto result = parse_packet(data, size);
    if (!result) return;

    const auto& [header, payload] = *result;

    switch (static_cast<PacketType>(header.type)) {
    case PacketType::Tracking: {
        if (payload.size() >= sizeof(TrackingFrame)) {
            TrackingFrame frame;
            std::memcpy(&frame, payload.data(), sizeof(TrackingFrame));
            if (on_tracking_frame) on_tracking_frame(frame);
        }
        break;
    }
    case PacketType::Video: {
        if (payload.size() >= sizeof(VideoPacket)) {
            VideoPacket packet;
            std::memcpy(&packet, payload.data(), sizeof(VideoPacket));
            if (on_video_packet) on_video_packet(packet);
        }
        break;
    }
    case PacketType::Control: {
        if (payload.size() >= sizeof(ControlMessage)) {
            ControlMessage msg;
            std::memcpy(&msg, payload.data(), sizeof(ControlMessage));
        }
        break;
    }
    default:
        break;
    }
}

} // namespace phonevr
