#include "input_sim.h"
#include "utils/logger.h"
#include "anti_detect.h"
#include <cmath>
#include <cstdlib>
#include <algorithm>

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

void InputSim::type_text(const std::string& text, const TypingConfig& config) {
    bool shift_down = false;

    for (size_t i = 0; i < text.size(); ++i) {
        char c = text[i];
        bool needs_shift = (c >= 'A' && c <= 'Z') ||
                           strchr("~!@#$%^&*()_+{}|:\"<>?", c);
        bool is_upper = (c >= 'A' && c <= 'Z');
        char lower = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));

        if (needs_shift && !shift_down) {
            key_down(VK_SHIFT);
            shift_down = true;
        } else if (!needs_shift && shift_down) {
            key_up(VK_SHIFT);
            shift_down = false;
        }

        DWORD vk;
        if (c >= 'a' && c <= 'z') {
            vk = 'A' + (c - 'a');
        } else if (c >= '0' && c <= '9') {
            vk = c;
        } else {
            vk = char_to_vk(c);
            if (vk == 0) continue;
        }

        key_down(vk);
        key_up(vk);

        int delay = AntiDetect::random_delay_range(config.min_delay_ms, config.max_delay_ms);
        Sleep(delay);
    }

    if (shift_down) {
        key_up(VK_SHIFT);
    }
}

DWORD InputSim::char_to_vk(char c) {
    switch (c) {
        case ' ': return VK_SPACE;
        case '.': return VK_OEM_PERIOD;
        case ',': return VK_OEM_COMMA;
        case ';': return VK_OEM_1;
        case '\'': return VK_OEM_7;
        case '[': return VK_OEM_4;
        case ']': return VK_OEM_6;
        case '\\': return VK_OEM_5;
        case '-': return VK_OEM_MINUS;
        case '=': return VK_OEM_PLUS;
        case '/': return VK_OEM_2;
        case '`': return VK_OEM_3;
        case '\n': case '\r': return VK_RETURN;
        case '\t': return VK_TAB;
        default: return 0;
    }
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

std::vector<MoveStep> InputSim::interpolate_bezier(int x1, int y1, int x2, int y2,
                                                     int steps, int step_delay_ms) {
    std::vector<MoveStep> result;
    if (steps <= 0) {
        result.push_back({x2, y2, 0});
        return result;
    }

    int dx = x2 - x1;
    int dy = y2 - y1;
    int dist = static_cast<int>(std::sqrt(dx * dx + dy * dy));

    int cp1x = x1 + dx / 3 + AntiDetect::random_offset() * 3;
    int cp1y = y1 + dy / 3 + AntiDetect::random_offset() * 3;
    int cp2x = x1 + dx * 2 / 3 + AntiDetect::random_offset() * 3;
    int cp2y = y1 + dy * 2 / 3 + AntiDetect::random_offset() * 3;

    for (int i = 1; i <= steps; ++i) {
        double t = static_cast<double>(i) / steps;
        double u = 1.0 - t;

        double xt = u * u * u * x1 + 3 * u * u * t * cp1x + 3 * u * t * t * cp2x + t * t * t * x2;
        double yt = u * u * u * y1 + 3 * u * u * t * cp1y + 3 * u * t * t * cp2y + t * t * t * y2;

        MoveStep step;
        step.x = static_cast<int>(std::round(xt));
        step.y = static_cast<int>(std::round(yt));
        step.delay_ms = step_delay_ms + (rand() % 3 - 1);
        if (step.delay_ms < 1) step.delay_ms = 1;
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
