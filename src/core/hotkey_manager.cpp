#include "hotkey_manager.h"
#include "utils/logger.h"

HotkeyManager::HotkeyManager() : hwnd_(nullptr), next_id_(100) {}

HotkeyManager::~HotkeyManager() {
    unregister_all();
}

bool HotkeyManager::register_hotkey(HotkeyAction action, UINT modifiers, UINT vk) {
    if (!hwnd_) {
        LOG_ERROR("HotkeyManager: HWND not set");
        return false;
    }

    UINT id = next_id_++;
    if (!RegisterHotKey(hwnd_, id, modifiers, vk)) {
        LOG_ERROR("RegisterHotKey failed for action {}: {}", static_cast<int>(action), GetLastError());
        return false;
    }

    hotkeys_[id] = action;
    LOG_INFO("Hotkey registered: {} (modifiers=0x{:x}, vk=0x{:x})",
             action_name(action), modifiers, vk);
    return true;
}

void HotkeyManager::unregister_all() {
    for (const auto& [id, _] : hotkeys_) {
        UnregisterHotKey(hwnd_, id);
    }
    hotkeys_.clear();
    LOG_INFO("All hotkeys unregistered");
}

bool HotkeyManager::handle_hotkey(UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg != WM_HOTKEY) return false;

    auto it = hotkeys_.find(static_cast<UINT>(wParam));
    if (it == hotkeys_.end()) return false;

    if (callback_) {
        callback_(it->second);
    }
    return true;
}

void HotkeyManager::set_callback(HotkeyCallback cb) {
    callback_ = std::move(cb);
}

void HotkeyManager::set_hwnd(HWND hwnd) {
    hwnd_ = hwnd;
}

const char* HotkeyManager::action_name(HotkeyAction action) {
    switch (action) {
        case HotkeyAction::ToggleRecording: return "Toggle Recording";
        case HotkeyAction::StopPlayback: return "Stop Playback";
        default: return "None";
    }
}
