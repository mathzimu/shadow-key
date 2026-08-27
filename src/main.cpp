#include "utils/logger.h"
#include "ui/main_window.h"
#include <cstdlib>
#include <ctime>

#if defined(_WIN32)
#include <windows.h>

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    srand(static_cast<unsigned>(time(nullptr)) + GetCurrentProcessId());
    Logger::init("shadow_key.log");
    LOG_INFO("ShadowKey starting...");
#else
int main() {
    srand(static_cast<unsigned>(time(nullptr)) + static_cast<unsigned>(getpid()));
    Logger::init("shadow_key.log");
    LOG_INFO("ShadowKey starting...");
#endif

    MainWindow window;
    if (!window.create()) {
        LOG_CRITICAL("Failed to create main window");
        return 1;
    }

    LOG_INFO("ShadowKey entering main loop");
    return window.run();
}
