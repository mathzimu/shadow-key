#pragma once
#include "input_hook.h"
#include <windows.h>
#include <vector>
#include <cstdint>

struct MoveStep {
    int x;
    int y;
    int delay_ms;
};

class InputSim {
public:
    static void key_down(DWORD vk_code);
    static void key_up(DWORD vk_code);
    static void key_press(DWORD vk_code, int hold_ms = 50);

    static void mouse_move(int x, int y);
    static void mouse_move_relative(int dx, int dy);
    static void mouse_left_click(int x, int y);
    static void mouse_right_click(int x, int y);
    static void mouse_left_down();
    static void mouse_left_up();
    static void mouse_scroll(int delta);

    static std::vector<MoveStep> interpolate_linear(int x1, int y1, int x2, int y2,
                                                      int steps, int step_delay_ms);

    static void play_events(const std::vector<InputEvent>& events);

private:
    static void send_input(INPUT& in);
};
