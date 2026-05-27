#include "input_hook.h"
#include "utils/logger.h"

InputHook* InputHook::instance_ = nullptr;

InputHook::InputHook()
    : keyboard_hook_(nullptr), mouse_hook_(nullptr), running_(false) {}

InputHook::~InputHook() {
    stop();
}

bool InputHook::start(InputEventCallback callback) {
    if (running_) {
        LOG_WARN("InputHook already running");
        return true;
    }

    callback_ = std::move(callback);
    instance_ = this;

    keyboard_hook_ = SetWindowsHookEx(WH_KEYBOARD_LL, keyboard_proc,
                                       GetModuleHandle(nullptr), 0);
    if (!keyboard_hook_) {
        LOG_ERROR("SetWindowsHookEx(WH_KEYBOARD_LL) failed: {}", GetLastError());
        return false;
    }

    mouse_hook_ = SetWindowsHookEx(WH_MOUSE_LL, mouse_proc,
                                    GetModuleHandle(nullptr), 0);
    if (!mouse_hook_) {
        LOG_ERROR("SetWindowsHookEx(WH_MOUSE_LL) failed: {}", GetLastError());
        UnhookWindowsHookEx(keyboard_hook_);
        keyboard_hook_ = nullptr;
        return false;
    }

    running_ = true;
    LOG_INFO("InputHook started");
    return true;
}

void InputHook::stop() {
    if (!running_) return;

    if (keyboard_hook_) {
        UnhookWindowsHookEx(keyboard_hook_);
        keyboard_hook_ = nullptr;
    }
    if (mouse_hook_) {
        UnhookWindowsHookEx(mouse_hook_);
        mouse_hook_ = nullptr;
    }

    running_ = false;
    instance_ = nullptr;
    LOG_INFO("InputHook stopped");
}

bool InputHook::is_running() const {
    return running_;
}

LRESULT CALLBACK InputHook::keyboard_proc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode >= 0 && instance_) {
        auto* kbd = reinterpret_cast<KBDLLHOOKSTRUCT*>(lParam);
        InputEvent ev{};
        ev.timestamp = GetTickCount();

        if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) {
            ev.type = InputEventType::KeyDown;
            ev.key.vk_code = kbd->vkCode;
            instance_->push_event(ev);
        } else if (wParam == WM_KEYUP || wParam == WM_SYSKEYUP) {
            ev.type = InputEventType::KeyUp;
            ev.key.vk_code = kbd->vkCode;
            instance_->push_event(ev);
        }
    }
    return CallNextHookEx(nullptr, nCode, wParam, lParam);
}

LRESULT CALLBACK InputHook::mouse_proc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode >= 0 && instance_) {
        auto* ms = reinterpret_cast<MSLLHOOKSTRUCT*>(lParam);
        InputEvent ev{};
        ev.timestamp = GetTickCount();

        switch (wParam) {
            case WM_MOUSEMOVE:
                ev.type = InputEventType::MouseMove;
                ev.mouse_move.x = ms->pt.x;
                ev.mouse_move.y = ms->pt.y;
                break;
            case WM_LBUTTONDOWN:
                ev.type = InputEventType::MouseLeftDown;
                ev.mouse_click.x = ms->pt.x;
                ev.mouse_click.y = ms->pt.y;
                break;
            case WM_LBUTTONUP:
                ev.type = InputEventType::MouseLeftUp;
                ev.mouse_click.x = ms->pt.x;
                ev.mouse_click.y = ms->pt.y;
                break;
            case WM_RBUTTONDOWN:
                ev.type = InputEventType::MouseRightDown;
                ev.mouse_click.x = ms->pt.x;
                ev.mouse_click.y = ms->pt.y;
                break;
            case WM_RBUTTONUP:
                ev.type = InputEventType::MouseRightUp;
                ev.mouse_click.x = ms->pt.x;
                ev.mouse_click.y = ms->pt.y;
                break;
            case WM_MOUSEWHEEL:
                ev.type = InputEventType::MouseWheel;
                ev.wheel.delta = GET_WHEEL_DELTA_WPARAM(ms->mouseData);
                break;
            default:
                return CallNextHookEx(nullptr, nCode, wParam, lParam);
        }

        instance_->push_event(ev);
    }
    return CallNextHookEx(nullptr, nCode, wParam, lParam);
}

void InputHook::push_event(const InputEvent& ev) {
    if (callback_) {
        callback_(ev);
    }
}
