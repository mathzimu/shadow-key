#pragma once
#include "input_hook.h"
#include <windows.h>
#include <vector>
#include <cstdint>
#include <string>

/// One step in an interpolated mouse-move sequence.
struct MoveStep {
    int x;
    int y;
    int delay_ms;
};

/// Configuration for simulated text typing.
struct TypingConfig {
    int  min_delay_ms  = 30;
    int  max_delay_ms  = 120;
    bool simulate_error = false;
    double error_rate  = 0.02;
};

/// Synthetic input generator.
///
/// Wraps SendInput to simulate keyboard and mouse actions with
/// optional anti-detection features (interpolation, random delay).
class InputSim {
public:
    InputSim() = delete;

    // -- Keyboard -----------------------------------------------------------

    static void key_down(DWORD vk_code) noexcept;
    static void key_up(DWORD vk_code) noexcept;
    static void key_press(DWORD vk_code, int hold_ms = 50) noexcept;

    // -- Mouse --------------------------------------------------------------

    static void mouse_move(int x, int y) noexcept;
    static void mouse_move_relative(int dx, int dy) noexcept;
    static void mouse_left_click(int x, int y) noexcept;
    static void mouse_right_click(int x, int y) noexcept;
    static void mouse_left_down() noexcept;
    static void mouse_left_up() noexcept;
    static void mouse_scroll(int delta) noexcept;

    // -- Text ---------------------------------------------------------------

    /// Type a string one character at a time with human-like delays.
    static void type_text(const std::string& text,
                          const TypingConfig& config = {});

    // -- Interpolation ------------------------------------------------------

    /// Linearly interpolated mouse-move steps.
    [[nodiscard]] static std::vector<MoveStep>
    interpolate_linear(int x1, int y1, int x2, int y2,
                       int steps, int step_delay_ms);

    /// Cubic-Bezier interpolated mouse-move steps with randomised control points.
    [[nodiscard]] static std::vector<MoveStep>
    interpolate_bezier(int x1, int y1, int x2, int y2,
                       int steps, int step_delay_ms);

    // -- Bulk playback ------------------------------------------------------

    /// Replay a sequence of InputEvents in order.
    static void play_events(const std::vector<InputEvent>& events);

private:
    static void send_input(INPUT& in) noexcept;
    static DWORD char_to_vk(char c) noexcept;
};
