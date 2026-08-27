#pragma once
#include "platform_types.h"

#if defined(_WIN32)
    #include <windows.h>
#endif

/// Low-level input recorder.
///
/// On Windows it installs WH_KEYBOARD_LL / WH_MOUSE_LL hooks and forwards
/// captured events through a user-supplied callback. On macOS it installs a
/// CGEvent tap on a background thread.
class InputHook {
public:
    InputHook() noexcept;
    ~InputHook();

    InputHook(const InputHook&)            = delete;
    InputHook& operator=(const InputHook&) = delete;

    /// Start capturing input.  Callback may run on a capture thread.
    /// @return true if capture started successfully.
    [[nodiscard]] bool start(InputEventCallback callback);

    /// Uninstall hooks/taps and stop capturing.
    void stop() noexcept;

    /// @return true when capture is active.
    [[nodiscard]] bool is_running() const noexcept;

    /// Dispatch an event to the registered callback (used by the capture thread).
    void push_event(const InputEvent& ev);

private:
#if defined(_WIN32)
    static LRESULT CALLBACK keyboard_proc(int nCode, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK mouse_proc(int nCode, WPARAM wParam, LPARAM lParam);

    static InputHook* instance_;
    HHOOK             keyboard_hook_{nullptr};
    HHOOK             mouse_hook_{nullptr};
#elif defined(__APPLE__)
    static void* tap_thread_proc(void* arg);
    pthread_t   tap_thread_{0};
    void*       tap_run_loop_{nullptr};
    int         tap_ref_count_{0};
#endif

    InputEventCallback callback_;
    bool                running_{false};
};
