#include "LauncherWindow.h"
#include <windowsx.h>
#include <commctrl.h>

#pragma comment(lib, "comctl32.lib")

namespace phonevr {

LauncherWindow::LauncherWindow() = default;
LauncherWindow::~LauncherWindow() = default;

bool LauncherWindow::create() {
    WNDCLASSEX wc = {};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.lpfnWndProc = LauncherWindow::window_proc;
    wc.hInstance = GetModuleHandle(nullptr);
    wc.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = L"PhoneVRLauncher";
    wc.hIconSm = LoadIcon(nullptr, IDI_APPLICATION);

    RegisterClassEx(&wc);

    hwnd_ = CreateWindowEx(
        0, L"PhoneVRLauncher", L"PhoneVR Launcher",
        WS_OVERLAPPEDWINDOW & ~WS_MAXIMIZEBOX & ~WS_THICKFRAME,
        CW_USEDEFAULT, CW_USEDEFAULT, 500, 400,
        nullptr, nullptr, wc.hInstance, this);

    if (!hwnd_) return false;

    create_controls();
    return true;
}

void LauncherWindow::show() {
    ShowWindow(hwnd_, SW_SHOW);
    UpdateWindow(hwnd_);
}

void LauncherWindow::run_message_loop() {
    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

LRESULT CALLBACK LauncherWindow::window_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    LauncherWindow* self = nullptr;

    if (msg == WM_NCCREATE) {
        auto* cs = reinterpret_cast<CREATESTRUCT*>(lparam);
        self = static_cast<LauncherWindow*>(cs->lpCreateParams);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
    } else {
        self = reinterpret_cast<LauncherWindow*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    }

    if (self) {
        return self->handle_message(msg, wparam, lparam);
    }

    return DefWindowProc(hwnd, msg, wparam, lparam);
}

LRESULT LauncherWindow::handle_message(UINT msg, WPARAM wparam, LPARAM lparam) {
    switch (msg) {
    case WM_CREATE:
        return 0;

    case WM_COMMAND:
        if (HIWORD(wparam) == BN_CLICKED) {
            auto btn_id = LOWORD(wparam);
            if (btn_id == 100) on_connect();
            else if (btn_id == 101) on_disconnect();
            else if (btn_id == 102) on_start_driver();
        }
        return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProc(hwnd_, msg, wparam, lparam);
}

void LauncherWindow::create_controls() {
    // Connection group
    CreateWindow(L"BUTTON", L"Connection", WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
                 10, 10, 460, 120, hwnd_, nullptr, GetModuleHandle(nullptr), nullptr);

    CreateWindow(L"STATIC", L"IP Address:", WS_CHILD | WS_VISIBLE,
                 20, 35, 80, 22, hwnd_, nullptr, GetModuleHandle(nullptr), nullptr);

    ip_edit_ = CreateWindow(L"EDIT", L"192.168.1.100",
                            WS_CHILD | WS_VISIBLE | WS_BORDER | ES_LEFT,
                            110, 35, 150, 22, hwnd_, nullptr, GetModuleHandle(nullptr), nullptr);

    CreateWindow(L"STATIC", L"Port:", WS_CHILD | WS_VISIBLE,
                 20, 65, 80, 22, hwnd_, nullptr, GetModuleHandle(nullptr), nullptr);

    port_edit_ = CreateWindow(L"EDIT", L"8765",
                              WS_CHILD | WS_VISIBLE | WS_BORDER | ES_NUMBER,
                              110, 65, 100, 22, hwnd_, nullptr, GetModuleHandle(nullptr), nullptr);

    connect_btn_ = CreateWindow(L"BUTTON", L"Connect",
                                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                                20, 95, 100, 28, hwnd_, (HMENU)100, GetModuleHandle(nullptr), nullptr);

    disconnect_btn_ = CreateWindow(L"BUTTON", L"Disconnect",
                                   WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                                130, 95, 100, 28, hwnd_, (HMENU)101, GetModuleHandle(nullptr), nullptr);

    // Driver group
    CreateWindow(L"BUTTON", L"SteamVR Driver", WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
                 10, 140, 460, 80, hwnd_, nullptr, GetModuleHandle(nullptr), nullptr);

    start_driver_btn_ = CreateWindow(L"BUTTON", L"Start Driver",
                                     WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                                     20, 165, 120, 28, hwnd_, (HMENU)102, GetModuleHandle(nullptr), nullptr);

    // Status
    CreateWindow(L"BUTTON", L"Status", WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
                 10, 230, 460, 120, hwnd_, nullptr, GetModuleHandle(nullptr), nullptr);

    status_text_ = CreateWindow(L"STATIC", L"Ready",
                                WS_CHILD | WS_VISIBLE | SS_LEFT,
                                20, 255, 440, 80, hwnd_, nullptr, GetModuleHandle(nullptr), nullptr);
}

void LauncherWindow::on_connect() {
    wchar_t ip_buf[64] = {};
    GetWindowText(ip_edit_, ip_buf, 64);
    char ip_str[64] = {};
    WideCharToMultiByte(CP_UTF8, 0, ip_buf, -1, ip_str, 64, nullptr, nullptr);
    current_ip_ = ip_str;

    wchar_t port_buf[16] = {};
    GetWindowText(port_edit_, port_buf, 16);
    current_port_ = static_cast<uint16_t>(_wtoi(port_buf));

    update_status("Connecting to " + current_ip_ + ":" + std::to_string(current_port_) + "...");
}

void LauncherWindow::on_disconnect() {
    update_status("Disconnected");
}

void LauncherWindow::on_start_driver() {
    update_status("Starting SteamVR driver...");
}

void LauncherWindow::update_status(const std::string& status) {
    SetWindowTextA(status_text_, status.c_str());
}

} // namespace phonevr
