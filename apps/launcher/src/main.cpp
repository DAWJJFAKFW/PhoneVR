#include "LauncherWindow.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
    phonevr::LauncherWindow window;

    if (!window.create()) {
        return 1;
    }

    window.show();
    window.run_message_loop();

    return 0;
}
