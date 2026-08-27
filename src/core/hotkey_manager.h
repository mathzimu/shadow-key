#pragma once
#include "platform_types.h"
#include <functional>
#include <unordered_map>

/// Actions that can be bound to global hotkeys.
enum class HotkeyAction {
    ToggleRecording,
    StopPlayback,
    None
};

/// Manages global hotkeys.
///
/// On Windows this wraps RegisterHotKey + WM_HOTKEY dispatch. On macOS it
/// uses Carbon hot-key registration and forwards actions to the callback
/// on the main thread.
class HotkeyManager {
public:
    using HotkeyCallback = std::function<void(HotkeyAction)>;

    HotkeyManager() noexcept;
    ~HotkeyManager();

    HotkeyManager(const HotkeyManager&)            = delete;
    HotkeyManager& operator=(const HotkeyManager&) = delete;

    /// Bind a hotkey combination to an action.
    /// @param modifiers  MOD_ALT | MOD_CONTROL | MOD_SHIFT | MOD_WIN.
    /// @param vk         Virtual-key code (e.g. 'R', 'S').
    /// @return true on success.
    [[nodiscard]] bool register_hotkey(HotkeyAction action,
                                        UINT modifiers, UINT vk);

    /// Unregister every previously registered hotkey.
    void unregister_all() noexcept;

#if defined(_WIN32)
    /// Call from WndProc to dispatch WM_HOTKEY messages.
    /// @return true if the message was consumed.
    bool handle_hotkey(UINT msg, WPARAM wParam, LPARAM lParam);
#endif

    void set_callback(HotkeyCallback cb) noexcept { callback_ = std::move(cb); }
    void set_hwnd(HWND hwnd) noexcept { hwnd_ = hwnd; }

    /// Human-readable name for a hotkey action.
    [[nodiscard]] static const char* action_name(HotkeyAction action) noexcept;

private:
    HWND                                        hwnd_{nullptr};
    HotkeyCallback                              callback_;
    std::unordered_map<UINT, HotkeyAction>      hotkeys_;
    UINT                                        next_id_{100};

#if defined(__APPLE__)
    struct MacHotkey {
        void* ref;        ///< EventHotKeyRef
        UINT  id;         ///< our assigned id
        HotkeyAction action;
    };
    std::vector<MacHotkey> mac_hotkeys_;
    void* event_handler_{nullptr};   ///< EventHandlerRef

    // Forward declarations of Carbon types so the friend declaration compiles
    // without pulling Carbon into every translation unit that includes this header.
    typedef struct OpaqueEventHandlerCallRef* EventHandlerCallRef;
    typedef struct OpaqueEventRef* EventRef;
    friend int mac_hotkey_handler(EventHandlerCallRef, EventRef, void*);
#endif
};
