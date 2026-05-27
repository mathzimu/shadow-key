#pragma once
#include <vector>
#include <cstdint>
#include <utility>

/// Random-delay distribution mode.
enum class DelayMode {
    Uniform,
    Gaussian
};

/// Mouse-movement curve mode.
enum class MouseCurveMode {
    Linear,
    Bezier
};

/// Master configuration for all anti-detection behaviour.
struct AntiDetectConfig {
    // -- Operation delay ----------------------------------------------------
    int  min_delay_ms           = 50;
    int  max_delay_ms           = 300;
    DelayMode delay_mode        = DelayMode::Uniform;

    // -- Mouse behaviour ---------------------------------------------------
    int  click_offset_px        = 5;
    int  mouse_move_steps_min   = 5;
    int  mouse_move_steps_max   = 15;
    int  mouse_step_delay_ms    = 8;
    MouseCurveMode curve_mode   = MouseCurveMode::Bezier;

    // -- Typing simulation -------------------------------------------------
    int  typing_min_delay_ms    = 30;
    int  typing_max_delay_ms    = 120;

    // -- Image matching ----------------------------------------------------
    int  screenshot_interval_ms = 500;

    // -- Recording ---------------------------------------------------------
    bool record_filter_mousemove = false;
};

/// Anti-detection utilities: random delays, offsets, and humanising curves.
class AntiDetect {
public:
    AntiDetect() = delete;

    /// Mutable global configuration singleton.
    [[nodiscard]] static AntiDetectConfig& config() noexcept;

    // -- Delay helpers ------------------------------------------------------

    /// Random delay uniformly sampled from [min_delay_ms, max_delay_ms].
    static int random_delay() noexcept;

    /// Random delay from [min_ms, max_ms].
    static int random_delay_range(int min_ms, int max_ms) noexcept;

    /// Random offset in [-click_offset_px, +click_offset_px].
    static int random_offset() noexcept;

    /// Uniform random integer in [min, max].
    static int random_int(int min, int max) noexcept;

    // -- Click offset -------------------------------------------------------

    /// Apply random pixel offset to a click position.
    [[nodiscard]] static std::vector<std::pair<int, int>>
    apply_click_offset(int x, int y);

    // -- Mouse steps --------------------------------------------------------

    /// Estimate a plausible number of interpolation steps for a given distance.
    static int get_mouse_steps(int distance) noexcept;

    // -- Gaussian sampling --------------------------------------------------

    /// Box-Muller normal sample.
    static double gaussian_sample(double mean, double stddev) noexcept;

private:
    static AntiDetectConfig config_;
};
