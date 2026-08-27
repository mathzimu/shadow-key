#pragma once
#include <cstdint>
#include <functional>

/// Platform-neutral types.
///
/// On Windows this pulls in <windows.h> so the existing Win32 code keeps
/// working. On other platforms we provide minimal stand-ins so the core
/// engine (recording/playback/script/image-matching/anti-detect) can build
/// without the Windows SDK.

#if defined(_WIN32)
    #include <windows.h>
#else
    using DWORD     = uint32_t;
    using UINT      = uint32_t;
    using BOOL      = int;
    using HWND      = void*;
    using HINSTANCE = void*;
    using WPARAM    = uintptr_t;
    using LPARAM    = intptr_t;
    using LRESULT   = intptr_t;

    #define MOD_ALT     0x0001
    #define MOD_CONTROL 0x0002
    #define MOD_SHIFT   0x0004
    #define MOD_WIN     0x0008
#endif

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
