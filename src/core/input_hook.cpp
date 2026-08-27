#include "input_hook.h"
#include "utils/logger.h"

#if defined(_WIN32)

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

#elif defined(__APPLE__)

#include <CoreGraphics/CoreGraphics.h>
#include <pthread.h>
#include <chrono>
#include <thread>

InputHook::InputHook() noexcept : running_(false) {}

InputHook::~InputHook() {
    stop();
}

bool InputHook::start(InputEventCallback callback) {
    if (running_) {
        LOG_WARN("InputHook already running");
        return true;
    }
    callback_ = std::move(callback);

    if (pthread_create(&tap_thread_, nullptr, tap_thread_proc, this) != 0) {
        LOG_ERROR("InputHook: failed to spawn capture thread");
        return false;
    }

    // Wait briefly for the tap to be installed.
    for (int i = 0; i < 50 && !running_; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    if (!running_) {
        LOG_ERROR("InputHook: event tap was not installed (check Accessibility permission)");
        return false;
    }
    LOG_INFO("InputHook started (macOS CGEventTap)");
    return true;
}

void InputHook::stop() noexcept {
    if (!running_) return;
    running_ = false;

    if (tap_run_loop_) {
        CFRunLoopStop(static_cast<CFRunLoopRef>(tap_run_loop_));
    }
    if (tap_thread_) {
        pthread_join(tap_thread_, nullptr);
        tap_thread_ = 0;
    }
    tap_run_loop_ = nullptr;
    LOG_INFO("InputHook stopped");
}

bool InputHook::is_running() const noexcept {
    return running_;
}

void InputHook::push_event(const InputEvent& ev) {
    if (callback_) callback_(ev);
}

static CGEventRef InputHook_tap_callback(CGEventTapProxy, CGEventType type,
                                         CGEventRef event, void* refcon) {
    auto* self = static_cast<InputHook*>(refcon);
    if (!self || !self->is_running()) return event;

    InputEvent ev{};
    ev.timestamp = static_cast<DWORD>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count());

    switch (type) {
        case kCGEventKeyDown: {
            ev.type = InputEventType::KeyDown;
            ev.key.vk_code = static_cast<DWORD>(
                CGEventGetIntegerValueField(event, kCGKeyboardEventKeycode));
            break;
        }
        case kCGEventKeyUp: {
            ev.type = InputEventType::KeyUp;
            ev.key.vk_code = static_cast<DWORD>(
                CGEventGetIntegerValueField(event, kCGKeyboardEventKeycode));
            break;
        }
        case kCGEventMouseMoved: {
            CGPoint p = CGEventGetLocation(event);
            ev.type = InputEventType::MouseMove;
            ev.mouse_move.x = static_cast<int>(p.x);
            ev.mouse_move.y = static_cast<int>(p.y);
            break;
        }
        case kCGEventLeftMouseDown: {
            CGPoint p = CGEventGetLocation(event);
            ev.type = InputEventType::MouseLeftDown;
            ev.mouse_click.x = static_cast<int>(p.x);
            ev.mouse_click.y = static_cast<int>(p.y);
            break;
        }
        case kCGEventLeftMouseUp: {
            CGPoint p = CGEventGetLocation(event);
            ev.type = InputEventType::MouseLeftUp;
            ev.mouse_click.x = static_cast<int>(p.x);
            ev.mouse_click.y = static_cast<int>(p.y);
            break;
        }
        case kCGEventRightMouseDown: {
            CGPoint p = CGEventGetLocation(event);
            ev.type = InputEventType::MouseRightDown;
            ev.mouse_click.x = static_cast<int>(p.x);
            ev.mouse_click.y = static_cast<int>(p.y);
            break;
        }
        case kCGEventRightMouseUp: {
            CGPoint p = CGEventGetLocation(event);
            ev.type = InputEventType::MouseRightUp;
            ev.mouse_click.x = static_cast<int>(p.x);
            ev.mouse_click.y = static_cast<int>(p.y);
            break;
        }
        case kCGEventScrollWheel: {
            int64_t d = CGEventGetIntegerValueField(
                event, kCGScrollWheelEventDeltaAxis1);
            ev.type = InputEventType::MouseWheel;
            ev.wheel.delta = static_cast<int>(d);
            break;
        }
        default:
            return event;
    }

    self->push_event(ev);
    return event;
}

void* InputHook::tap_thread_proc(void* arg) {
    auto* self = static_cast<InputHook*>(arg);

    CGEventMask mask = CGEventMaskBit(kCGEventKeyDown) |
                      CGEventMaskBit(kCGEventKeyUp) |
                      CGEventMaskBit(kCGEventMouseMoved) |
                      CGEventMaskBit(kCGEventLeftMouseDown) |
                      CGEventMaskBit(kCGEventLeftMouseUp) |
                      CGEventMaskBit(kCGEventRightMouseDown) |
                      CGEventMaskBit(kCGEventRightMouseUp) |
                      CGEventMaskBit(kCGEventScrollWheel);

    CFMachPortRef tap = CGEventTapCreate(
        kCGSessionEventTap, kCGHeadInsertEventTap, kCGEventTapOptionDefault,
        mask, InputHook_tap_callback, self);
    if (!tap) {
        LOG_ERROR("InputHook: CGEventTapCreate failed - grant Accessibility "
                  "permission in System Settings > Privacy & Security");
        return nullptr;
    }

    CFRunLoopSourceRef source = CFMachPortCreateRunLoopSource(
        kCFAllocatorDefault, tap, 0);
    CFRunLoopRef rl = CFRunLoopGetCurrent();
    self->tap_run_loop_ = rl;
    CFRunLoopAddSource(rl, source, kCFRunLoopCommonModes);

    self->running_ = true;
    CFRunLoopRun();

    CFRunLoopRemoveSource(rl, source, kCFRunLoopCommonModes);
    CFRelease(source);
    CFRelease(tap);
    return nullptr;
}

#endif
