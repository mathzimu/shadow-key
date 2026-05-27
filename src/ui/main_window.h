#pragma once
#include "core/input_hook.h"
#include "core/anti_detect.h"
#include "core/hotkey_manager.h"
#include "script/script_executor.h"
#include "script/script_format.h"
#include <windows.h>
#include <string>
#include <vector>
#include <atomic>
#include <mutex>

/// Main application window combining Dear ImGui UI, input hook, script
/// executor, and global-hotkey management.
class MainWindow {
public:
    MainWindow() noexcept;
    ~MainWindow();

    MainWindow(const MainWindow&)            = delete;
    MainWindow& operator=(const MainWindow&) = delete;

    /// Register the window class, create the window, and initialise ImGui.
    [[nodiscard]] bool create(HINSTANCE hInstance);

    /// Message loop.  Returns the exit code from PostQuitMessage.
    int run();

private:
    // -- Window handles -----------------------------------------------------
    HWND      hwnd_{nullptr};
    HINSTANCE hinstance_{nullptr};

    // -- Sub-systems --------------------------------------------------------
    InputHook       hook_;
    ScriptExecutor  executor_;
    HotkeyManager   hotkeys_;

    // -- Recording state ----------------------------------------------------
    std::vector<InputEvent> recorded_events_;
    std::atomic<bool>       is_recording_{false};
    std::string             current_script_path_;
    AntiDetectConfig&       anti_config_;

    // -- Thread-safe event buffer -------------------------------------------
    std::mutex              events_mutex_;
    std::vector<InputEvent> pending_events_;

    // -- UI editor state ----------------------------------------------------
    int     delete_action_idx_{-1};
    int     edit_delay_idx_{-1};
    int     edit_delay_value_{0};
    char    edit_text_buffer_[4096];
    int     edit_text_idx_{-1};
    float   speed_multiplier_{1.0f};
    char    script_name_buffer_[256];
    char    script_desc_buffer_[256];

    // -- Message handling ---------------------------------------------------
    static LRESULT CALLBACK wnd_proc(HWND hwnd, UINT msg,
                                     WPARAM wParam, LPARAM lParam);
    LRESULT handle_message(UINT msg, WPARAM wParam, LPARAM lParam);

    // -- ImGui helpers ------------------------------------------------------
    void render_ui();
    void setup_imgui();
    void cleanup_imgui();

    // -- Action handlers ----------------------------------------------------
    void on_start_recording();
    void on_stop_recording();
    void on_save_script();
    void on_load_script();
    void on_start_playback();
    void on_stop_playback();

    // -- Callbacks ----------------------------------------------------------
    void recording_callback(const InputEvent& event);
    void executor_status_callback(ExecutorState state, int index,
                                  const std::string& msg);

    // -- Tab renderers ------------------------------------------------------
    void render_main_tab();
    void render_settings_tab();
    void render_log_tab();
    void render_script_editor();
};
