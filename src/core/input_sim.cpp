#include "input_sim.h"
#include "utils/logger.h"
#include <cmath>
#include <cstdlib>

void InputSim::send_input(INPUT& in) {
    if (SendInput(1, &in, sizeof(INPUT)) != 1) {
        LOG_WARN("SendInput failed: {}", GetLastError());
    }
}

void InputSim::key_down(DWORD vk_code) {
    INPUT in = {};
    in.type = INPUT_KEYBOARD;
    in.ki.wVk = vk_code;
    send_input(in);
}

void InputSim::key_up(DWORD vk_code) {
    INPUT in = {};
    in.type = INPUT_KEYBOARD;
    in.ki.wVk = vk_code;
    in.ki.dwFlags = KEYEVENTF_KEYUP;
    send_input(in);
}

void InputSim::key_press(DWORD vk_code, int hold_ms) {
    key_down(vk_code);
    Sleep(hold_ms);
    key_up(vk_code);
}

void InputSim::mouse_move(int x, int y) {
    INPUT in = {};
    in.type = INPUT_MOUSE;
    in.mi.dx = static_cast<LONG>(x * 65535.0 / GetSystemMetrics(SM_CXSCREEN));
    in.mi.dy = static_cast<LONG>(y * 65535.0 / GetSystemMetrics(SM_CYSCREEN));
    in.mi.dwFlags = MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE;
    send_input(in);
}

void InputSim::mouse_move_relative(int dx, int dy) {
    INPUT in = {};
    in.type = INPUT_MOUSE;
    in.mi.dx = dx;
    in.mi.dy = dy;
    in.mi.dwFlags = MOUSEEVENTF_MOVE;
    send_input(in);
}

void InputSim::mouse_left_click(int x, int y) {
    mouse_move(x, y);
    mouse_left_down();
    Sleep(30 + rand() % 20);
    mouse_left_up();
}

void InputSim::mouse_right_click(int x, int y) {
    mouse_move(x, y);
    INPUT in = {};
    in.type = INPUT_MOUSE;
    in.mi.dwFlags = MOUSEEVENTF_RIGHTDOWN;
    send_input(in);
    Sleep(30 + rand() % 20);
    in.mi.dwFlags = MOUSEEVENTF_RIGHTUP;
    send_input(in);
}

void InputSim::mouse_left_down() {
    INPUT in = {};
    in.type = INPUT_MOUSE;
    in.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
    send_input(in);
}

void InputSim::mouse_left_up() {
    INPUT in = {};
    in.type = INPUT_MOUSE;
    in.mi.dwFlags = MOUSEEVENTF_LEFTUP;
    send_input(in);
}

void InputSim::mouse_scroll(int delta) {
    INPUT in = {};
    in.type = INPUT_MOUSE;
    in.mi.mouseData = static_cast<DWORD>(delta);
    in.mi.dwFlags = MOUSEEVENTF_WHEEL;
    send_input(in);
}

std::vector<MoveStep> InputSim::interpolate_linear(int x1, int y1, int x2, int y2,
                                                     int steps, int step_delay_ms) {
    std::vector<MoveStep> result;
    if (steps <= 0) {
        result.push_back({x2, y2, 0});
        return result;
    }
    for (int i = 1; i <= steps; ++i) {
        double t = static_cast<double>(i) / steps;
        MoveStep step;
        step.x = static_cast<int>(std::round(x1 + (x2 - x1) * t));
        step.y = static_cast<int>(std::round(y1 + (y2 - y1) * t));
        step.delay_ms = step_delay_ms;
        result.push_back(step);
    }
    return result;
}

void InputSim::play_events(const std::vector<InputEvent>& events) {
    int current_x = -1, current_y = -1;

    for (const auto& ev : events) {
        switch (ev.type) {
            case InputEventType::KeyDown:
                key_down(ev.key.vk_code);
                break;
            case InputEventType::KeyUp:
                key_up(ev.key.vk_code);
                break;
            case InputEventType::MouseMove:
                mouse_move(ev.mouse_move.x, ev.mouse_move.y);
                current_x = ev.mouse_move.x;
                current_y = ev.mouse_move.y;
                break;
            case InputEventType::MouseLeftDown:
                if (current_x < 0 || current_y < 0) {
                    mouse_left_down();
                } else {
                    mouse_left_click(ev.mouse_click.x, ev.mouse_click.y);
                }
                current_x = ev.mouse_click.x;
                current_y = ev.mouse_click.y;
                break;
            case InputEventType::MouseLeftUp:
                mouse_left_up();
                break;
            case InputEventType::MouseRightDown:
                mouse_right_click(ev.mouse_click.x, ev.mouse_click.y);
                current_x = ev.mouse_click.x;
                current_y = ev.mouse_click.y;
                break;
            case InputEventType::MouseRightUp:
                break;
            case InputEventType::MouseWheel:
                mouse_scroll(ev.wheel.delta);
                break;
            default:
                break;
        }
    }
}
