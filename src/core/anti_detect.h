#pragma once
#include <vector>
#include <cstdint>

enum class DelayMode {
    Uniform,
    Gaussian
};

enum class MouseCurveMode {
    Linear,
    Bezier
};

struct AntiDetectConfig {
    int min_delay_ms = 50;
    int max_delay_ms = 300;
    int click_offset_px = 5;
    int mouse_move_steps_min = 5;
    int mouse_move_steps_max = 15;
    int mouse_step_delay_ms = 8;
    int screenshot_interval_ms = 500;

    int typing_min_delay_ms = 30;
    int typing_max_delay_ms = 120;
    bool record_filter_mousemove = false;

    DelayMode delay_mode = DelayMode::Uniform;
    MouseCurveMode curve_mode = MouseCurveMode::Bezier;
};

class AntiDetect {
public:
    static AntiDetectConfig& config();

    static int random_delay();
    static int random_delay_range(int min_ms, int max_ms);
    static int random_offset();
    static int random_int(int min, int max);

    static std::vector<std::pair<int, int>> apply_click_offset(int x, int y);

    static int get_mouse_steps(int distance);

    static double gaussian_sample(double mean, double stddev);

private:
    static AntiDetectConfig config_;
};
