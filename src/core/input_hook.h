#pragma once
#include <windows.h>
#include <functional>

enum class InputEventType {
    KeyDown,
    KeyUp,
    MouseMove,
    MouseLeftDown,
    MouseLeftUp,
    MouseRightDown,
    MouseRightUp,
    MouseWheel
};

struct InputEvent {
    InputEventType type;
    DWORD timestamp;
    union {
        struct { DWORD vk_code; } key;
        struct { int x; int y; } mouse_move;
        struct { int x; int y; } mouse_click;
        struct { int delta; } wheel;
    };
};

using InputEventCallback = std::function<void(const InputEvent&)>;

class InputHook {
public:
    InputHook();
    ~InputHook();

    bool start(InputEventCallback callback);
    void stop();
    bool is_running() const;

private:
    static LRESULT CALLBACK keyboard_proc(int nCode, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK mouse_proc(int nCode, WPARAM wParam, LPARAM lParam);

    static InputHook* instance_;
    HHOOK keyboard_hook_;
    HHOOK mouse_hook_;
    InputEventCallback callback_;
    bool running_;

    void push_event(const InputEvent& ev);
};
