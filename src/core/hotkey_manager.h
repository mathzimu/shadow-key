#pragma once
#include <windows.h>
#include <functional>
#include <unordered_map>

enum class HotkeyAction {
    ToggleRecording,
    StopPlayback,
    None
};

class HotkeyManager {
public:
    using HotkeyCallback = std::function<void(HotkeyAction)>;

    HotkeyManager();
    ~HotkeyManager();

    bool register_hotkey(HotkeyAction action, UINT modifiers, UINT vk);
    void unregister_all();

    bool handle_hotkey(UINT msg, WPARAM wParam, LPARAM lParam);

    void set_callback(HotkeyCallback cb);
    void set_hwnd(HWND hwnd);

    static const char* action_name(HotkeyAction action);

private:
    HWND hwnd_;
    HotkeyCallback callback_;
    std::unordered_map<UINT, HotkeyAction> hotkeys_;
    UINT next_id_;
};
