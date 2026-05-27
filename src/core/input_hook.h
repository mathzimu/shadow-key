#pragma once
#include <windows.h>
#include <functional>

/// Types of input events that can be recorded or scripted.
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

/// A single recorded input event.
struct InputEvent {
    InputEventType type;
    DWORD          timestamp;          ///< Tick count at capture time.
    union {
        struct { DWORD vk_code; }          key;         ///< Virtual-key code for keyboard events.
        struct { int x; int y; }           mouse_move;  ///< Cursor position for mouse-move events.
        struct { int x; int y; }           mouse_click; ///< Cursor position for click events.
        struct { int delta; }              wheel;       ///< Wheel delta for scroll events.
    };
};

/// Callback invoked on every captured input event.
using InputEventCallback = std::function<void(const InputEvent&)>;

/// Low-level Windows hook recorder.
///
/// Installs WH_KEYBOARD_LL and WH_MOUSE_LL hooks and forwards
/// captured events through a user-supplied callback.
class InputHook {
public:
    InputHook() noexcept;
    ~InputHook();

    InputHook(const InputHook&)            = delete;
    InputHook& operator=(const InputHook&) = delete;

    /// Start capturing input.  Callback runs on the message-pump thread.
    /// @return true if both hooks were installed successfully.
    [[nodiscard]] bool start(InputEventCallback callback);

    /// Uninstall hooks and stop capturing.
    void stop() noexcept;

    /// @return true when hooks are active.
    [[nodiscard]] bool is_running() const noexcept;

private:
    static LRESULT CALLBACK keyboard_proc(int nCode, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK mouse_proc(int nCode, WPARAM wParam, LPARAM lParam);

    static InputHook* instance_;
    HHOOK             keyboard_hook_{nullptr};
    HHOOK             mouse_hook_{nullptr};
    InputEventCallback callback_;
    bool              running_{false};
};
