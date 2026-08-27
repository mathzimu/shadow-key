#include "hotkey_manager.h"
#include "utils/logger.h"

#if defined(_WIN32)

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

#elif defined(__APPLE__)

#include <Carbon/Carbon.h>
#include <dispatch/dispatch.h>

HotkeyManager::HotkeyManager() noexcept = default;

HotkeyManager::~HotkeyManager() {
    unregister_all();
}

static CGKeyCode mac_vk_to_keycode(UINT vk) {
    switch (vk) {
        case 'R': return 0x0F; // kVK_ANSI_R
        case 'S': return 0x01; // kVK_ANSI_S
        case 'A': return 0x00;
        case 'B': return 0x0B;
        case 'C': return 0x08;
        case 'D': return 0x02;
        case 'E': return 0x0E;
        case 'F': return 0x03;
        case 'P': return 0x23;
        case 'Q': return 0x0C;
        case 'W': return 0x0D;
        default:  return static_cast<CGKeyCode>(vk);
    }
}

static UInt32 mac_modifiers(UINT modifiers) {
    UInt32 m = 0;
    if (modifiers & MOD_CONTROL) m |= controlKey;
    if (modifiers & MOD_ALT)     m |= optionKey;
    if (modifiers & MOD_SHIFT)   m |= shiftKey;
    if (modifiers & MOD_WIN)     m |= cmdKey;
    return m;
}

OSStatus mac_hotkey_handler(EventHandlerCallRef, EventRef event, void* userData) {
    auto* self = static_cast<HotkeyManager*>(userData);
    if (!self) return eventNotHandledErr;

    EventHotKeyID hkID;
    GetEventParameter(event, kEventParamDirectObject, typeEventHotKeyID,
                      nullptr, sizeof(hkID), nullptr, &hkID);

    HotkeyAction action = HotkeyAction::None;
    for (const auto& h : self->mac_hotkeys_) {
        if (h.id == hkID.id) { action = h.action; break; }
    }
    if (action == HotkeyAction::None) return eventNotHandledErr;

    // Forward on the main thread to keep UI code single-threaded.
    if (self->callback_) {
        dispatch_async(dispatch_get_main_queue(), ^{
            self->callback_(action);
        });
    }
    return noErr;
}

bool HotkeyManager::register_hotkey(HotkeyAction action, UINT modifiers, UINT vk) {
    UINT id = next_id_++;

    EventHotKeyID hkID = { 'SKHK', static_cast<UInt32>(id) };
    EventHotKeyRef ref = nullptr;
    OSStatus err = RegisterEventHotKey(mac_vk_to_keycode(vk), mac_modifiers(modifiers),
                                       hkID, GetApplicationEventTarget(), 0, &ref);
    if (err != noErr || !ref) {
        LOG_ERROR("HotkeyManager: RegisterEventHotKey failed for {} (macOS keycode={})",
                  action_name(action), static_cast<int>(mac_vk_to_keycode(vk)));
        return false;
    }

    if (!event_handler_) {
        EventTypeSpec spec = { kEventClassKeyboard, kEventHotKeyPressed };
        InstallApplicationEventHandler(&mac_hotkey_handler, 1, &spec, this, nullptr);
    }

    mac_hotkeys_.push_back({ref, id, action});
    LOG_INFO("Hotkey registered (macOS): {}", action_name(action));
    return true;
}

void HotkeyManager::unregister_all() noexcept {
    for (const auto& h : mac_hotkeys_) {
        if (h.ref) UnregisterEventHotKey(static_cast<EventHotKeyRef>(h.ref));
    }
    mac_hotkeys_.clear();
    LOG_INFO("All hotkeys unregistered");
}

#endif

const char* HotkeyManager::action_name(HotkeyAction action) noexcept {
    switch (action) {
        case HotkeyAction::ToggleRecording: return "Toggle Recording";
        case HotkeyAction::StopPlayback: return "Stop Playback";
        default: return "None";
    }
}
