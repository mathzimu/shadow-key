#pragma once
#include "core/input_hook.h"
#include "core/anti_detect.h"
#include "core/hotkey_manager.h"
#include "script/script_executor.h"
#include "script/script_format.h"
#include "core/platform_types.h"
#include <string>
#include <vector>
#include <atomic>
#include <mutex>

#if defined(__APPLE__)
    #include <GLFW/glfw3.h>
#endif

/// Main application window combining Dear ImGui UI, input hook, script
/// executor, and global-hotkey management.
class MainWindow {
public:
    MainWindow() noexcept;
    ~MainWindow();

    MainWindow(const MainWindow&)            = delete;
    MainWindow& operator=(const MainWindow&) = delete;

    /// Create the window and initialise ImGui.
    [[nodiscard]] bool create();

    /// Run the UI loop until the window is closed. Returns the exit code.
    int run();

private:
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

#if defined(_WIN32)
    HWND      hwnd_{nullptr};
    HINSTANCE hinstance_{nullptr};
#elif defined(__APPLE__)
    GLFWwindow* glfw_window_{nullptr};
#endif

    // -- ImGui helpers ------------------------------------------------------
    void render_ui();
    void setup_imgui();
    void cleanup_imgui();
    void build_imgui_frame();

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

#if defined(__APPLE__)
    void mac_open_file_dialog(std::string& out_path);
    void mac_save_file_dialog(std::string& out_path);
#endif
};
