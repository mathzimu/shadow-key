#pragma once
#include <windows.h>
#include <functional>
#include <unordered_map>

/// Actions that can be bound to global hotkeys.
enum class HotkeyAction {
    ToggleRecording,
    StopPlayback,
    None
};

/// Manages global hotkeys via RegisterHotKey.
class HotkeyManager {
public:
    using HotkeyCallback = std::function<void(HotkeyAction)>;

    HotkeyManager() noexcept;
    ~HotkeyManager();

    HotkeyManager(const HotkeyManager&)            = delete;
    HotkeyManager& operator=(const HotkeyManager&) = delete;

    /// Bind a hotkey combination to an action.
    /// @param modifiers  MOD_ALT | MOD_CONTROL | MOD_SHIFT | MOD_WIN.
    /// @param vk         Virtual-key code (e.g. 'R', VK_F1).
    /// @return true on success.
    [[nodiscard]] bool register_hotkey(HotkeyAction action,
                                        UINT modifiers, UINT vk);

    /// Unregister every previously registered hotkey.
    void unregister_all() noexcept;

    /// Call from WndProc to dispatch WM_HOTKEY messages.
    /// @return true if the message was consumed.
    bool handle_hotkey(UINT msg, WPARAM wParam, LPARAM lParam);

    void set_callback(HotkeyCallback cb) noexcept { callback_ = std::move(cb); }
    void set_hwnd(HWND hwnd) noexcept { hwnd_ = hwnd; }

    /// Human-readable name for a hotkey action.
    [[nodiscard]] static const char* action_name(HotkeyAction action) noexcept;

private:
    HWND                                        hwnd_{nullptr};
    HotkeyCallback                              callback_;
    std::unordered_map<UINT, HotkeyAction>      hotkeys_;
    UINT                                        next_id_{100};
};
