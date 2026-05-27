#include "utils/logger.h"
#include "ui/main_window.h"
#include <cstdlib>
#include <ctime>

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
    srand(static_cast<unsigned>(time(nullptr)) + GetCurrentProcessId());
    Logger::init("shadow_key.log");
    LOG_INFO("ShadowKey v{} starting...", "0.1.0");

    MainWindow window;
    if (!window.create(hInstance)) {
        LOG_CRITICAL("Failed to create main window");
        return 1;
    }

    LOG_INFO("ShadowKey entering message loop");
    return window.run();
}
