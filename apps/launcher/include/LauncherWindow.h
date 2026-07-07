#pragma once

#include <windows.h>
#include <string>

namespace phonevr {

class LauncherWindow {
public:
    LauncherWindow();
    ~LauncherWindow();

    bool create();
    void show();
    void run_message_loop();

private:
    static LRESULT CALLBACK window_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
    LRESULT handle_message(UINT msg, WPARAM wparam, LPARAM lparam);

    void create_controls();
    void on_connect();
    void on_disconnect();
    void on_start_driver();
    void update_status(const std::string& status);

    HWND hwnd_ = nullptr;

    HWND status_text_ = nullptr;
    HWND connect_btn_ = nullptr;
    HWND disconnect_btn_ = nullptr;
    HWND start_driver_btn_ = nullptr;

    HWND ip_edit_ = nullptr;
    HWND port_edit_ = nullptr;

    std::string current_ip_ = "192.168.1.100";
    uint16_t current_port_ = 8765;
};

} // namespace phonevr
