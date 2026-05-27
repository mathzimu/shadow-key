#pragma once
#include "core/input_hook.h"
#include "core/anti_detect.h"
#include "script/script_executor.h"
#include <windows.h>
#include <string>
#include <vector>
#include <atomic>
#include <thread>

class MainWindow {
public:
    MainWindow();
    ~MainWindow();

    bool create(HINSTANCE hInstance);
    int run();

private:
    HWND hwnd_;
    HINSTANCE hinstance_;
    InputHook hook_;
    ScriptExecutor executor_;
    std::vector<InputEvent> recorded_events_;
    std::atomic<bool> is_recording_;
    std::string current_script_path_;
    AntiDetectConfig& anti_config_;

    static LRESULT CALLBACK wnd_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    LRESULT handle_message(UINT msg, WPARAM wParam, LPARAM lParam);

    void render_ui();
    void setup_imgui();
    void cleanup_imgui();

    void on_start_recording();
    void on_stop_recording();
    void on_save_script();
    void on_load_script();
    void on_start_playback();
    void on_stop_playback();

    void recording_callback(const InputEvent& event);
    void executor_status_callback(ExecutorState state, int index, const std::string& msg);

    void render_main_tab();
    void render_settings_tab();
    void render_log_tab();
};
