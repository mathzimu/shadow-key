#include "anti_detect.h"
#include <cstdlib>
#include <ctime>
#include <cmath>
#include <algorithm>

#if defined(_WIN32)
    #include <windows.h>
#elif defined(__APPLE__)
    #include <unistd.h>
#else
    #include <unistd.h>
#endif

AntiDetectConfig AntiDetect::config_;

AntiDetectConfig& AntiDetect::config() noexcept {
    static bool seeded = false;
    if (!seeded) {
#if defined(_WIN32)
        srand(static_cast<unsigned>(time(nullptr)) + GetCurrentProcessId());
#else
        srand(static_cast<unsigned>(time(nullptr)) + static_cast<unsigned>(getpid()));
#endif
        seeded = true;
    }
    return config_;
}

int AntiDetect::random_delay() noexcept {
    return random_delay_range(config_.min_delay_ms, config_.max_delay_ms);
}

int AntiDetect::random_delay_range(int min_ms, int max_ms) noexcept {
    if (max_ms <= min_ms) return min_ms;
    return min_ms + rand() % (max_ms - min_ms + 1);
}

int AntiDetect::random_offset() noexcept {
    return rand() % (config_.click_offset_px * 2 + 1) - config_.click_offset_px;
}

int AntiDetect::random_int(int min, int max) noexcept {
    if (max <= min) return min;
    return min + rand() % (max - min + 1);
}

std::vector<std::pair<int, int>> AntiDetect::apply_click_offset(int x, int y) {
    int ox = x + random_offset();
    int oy = y + random_offset();
    return {{ox, oy}};
}

int AntiDetect::get_mouse_steps(int distance) noexcept {
    int steps = distance / 30;
    return std::clamp(steps, config_.mouse_move_steps_min, config_.mouse_move_steps_max);
}

double AntiDetect::gaussian_sample(double mean, double stddev) noexcept {
    static bool has_spare = false;
    static double spare;

    if (has_spare) {
        has_spare = false;
        return mean + stddev * spare;
    }

    double u, v, s;
    do {
        u = (rand() + 1.0) / (RAND_MAX + 1.0) * 2.0 - 1.0;
        v = (rand() + 1.0) / (RAND_MAX + 1.0) * 2.0 - 1.0;
        s = u * u + v * v;
    } while (s >= 1.0 || s == 0.0);

    s = std::sqrt(-2.0 * std::log(s) / s);
    spare = v * s;
    has_spare = true;

    return mean + stddev * u * s;
}
