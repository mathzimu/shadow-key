#include "main_window.h"
#include "utils/logger.h"
#include "script/script_format.h"
#include <commdlg.h>

#include <imgui.h>
#include <imgui_impl_win32.h>
#include <filesystem>
#include <sstream>
#include <iomanip>
#include <ctime>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

static std::string status_log;
static std::mutex log_mutex;

void add_log(const std::string& msg) {
    std::lock_guard<std::mutex> lock(log_mutex);
    auto t = std::time(nullptr);
    auto tm = *std::localtime(&t);
    std::ostringstream ss;
    ss << std::put_time(&tm, "%H:%M:%S") << " " << msg;
    status_log += ss.str() + "\n";
    if (status_log.size() > 65536) {
        status_log.erase(status_log.begin(), status_log.begin() + 32768);
    }
}

static std::string executor_state_str(ExecutorState s) {
    switch (s) {
        case ExecutorState::Idle: return "Idle";
        case ExecutorState::Running: return "Running";
        case ExecutorState::Paused: return "Paused";
        case ExecutorState::Stopped: return "Stopped";
        case ExecutorState::Error: return "Error";
        default: return "Unknown";
    }
}

static std::string event_desc(const ScriptAction& a) {
    switch (a.type) {
        case InputEventType::KeyDown:   return "KeyDown vk=" + std::to_string(a.key.vk_code);
        case InputEventType::KeyUp:     return "KeyUp vk=" + std::to_string(a.key.vk_code);
        case InputEventType::MouseMove: return "Move (" + std::to_string(a.mouse_move.x) + "," + std::to_string(a.mouse_move.y) + ")";
        case InputEventType::MouseLeftDown:  return "LeftClick (" + std::to_string(a.mouse_click.x) + "," + std::to_string(a.mouse_click.y) + ")";
        case InputEventType::MouseLeftUp:    return "LeftUp";
        case InputEventType::MouseRightDown: return "RightClick (" + std::to_string(a.mouse_click.x) + "," + std::to_string(a.mouse_click.y) + ")";
        case InputEventType::MouseRightUp:   return "RightUp";
        case InputEventType::MouseWheel:     return "Wheel delta=" + std::to_string(a.wheel.delta);
        default: return "Unknown";
    }
}

MainWindow::MainWindow()
    : hwnd_(nullptr), hinstance_(nullptr),
      is_recording_(false),
      anti_config_(AntiDetect::config()),
      delete_action_idx_(-1), edit_delay_idx_(-1), edit_delay_value_(0),
      edit_text_idx_(-1), speed_multiplier_(1.0f) {
    edit_text_buffer_[0] = '\0';
    script_name_buffer_[0] = '\0';
    script_desc_buffer_[0] = '\0';
}

MainWindow::~MainWindow() {
    hook_.stop();
    hotkeys_.unregister_all();
    cleanup_imgui();
}

bool MainWindow::create(HINSTANCE hInstance) {
    hinstance_ = hInstance;

    const wchar_t* CLASS_NAME = L"ShadowKeyWindow";

    WNDCLASSEX wc = {};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = wnd_proc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    wc.lpszClassName = CLASS_NAME;

    if (!RegisterClassEx(&wc)) {
        LOG_ERROR("RegisterClassEx failed: {}", GetLastError());
        return false;
    }

    RECT rect = {0, 0, 700, 600};
    AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);

    hwnd_ = CreateWindowEx(
        0, CLASS_NAME, L"ShadowKey 智能按键精灵",
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
        rect.right - rect.left, rect.bottom - rect.top,
        nullptr, nullptr, hInstance, this
    );

    if (!hwnd_) {
        LOG_ERROR("CreateWindowEx failed: {}", GetLastError());
        return false;
    }

    setup_imgui();
    ShowWindow(hwnd_, SW_SHOW);
    UpdateWindow(hwnd_);

    hotkeys_.set_hwnd(hwnd_);
    hotkeys_.register_hotkey(HotkeyAction::ToggleRecording, MOD_CONTROL | MOD_ALT, 'R');
    hotkeys_.register_hotkey(HotkeyAction::StopPlayback, MOD_CONTROL | MOD_ALT, 'S');
    hotkeys_.set_callback([this](HotkeyAction action) {
        switch (action) {
            case HotkeyAction::ToggleRecording:
                if (is_recording_) on_stop_recording();
                else on_start_recording();
                break;
            case HotkeyAction::StopPlayback:
                on_stop_playback();
                break;
        }
    });

    executor_.set_status_callback([this](ExecutorState state, int index, const std::string& msg) {
        executor_status_callback(state, index, msg);
    });

    LOG_INFO("ShadowKey window created");
    return true;
}

int MainWindow::run() {
    MSG msg;
    while (true) {
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            if (msg.message == WM_QUIT) return 0;
        }

        {
            std::lock_guard<std::mutex> lock(events_mutex_);
            for (const auto& ev : pending_events_) {
                recorded_events_.push_back(ev);
            }
            pending_events_.clear();
        }

        render_ui();
    }
    return 0;
}

LRESULT CALLBACK MainWindow::wnd_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam))
        return true;

    if (msg == WM_CREATE) {
        auto* cs = reinterpret_cast<CREATESTRUCT*>(lParam);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(cs->lpCreateParams));
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }

    auto* window = reinterpret_cast<MainWindow*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    if (window) {
        if (window->hotkeys_.handle_hotkey(msg, wParam, lParam))
            return 0;
        return window->handle_message(msg, wParam, lParam);
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

LRESULT MainWindow::handle_message(UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        default:
            return DefWindowProc(hwnd_, msg, wParam, lParam);
    }
}

void MainWindow::setup_imgui() {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.FontGlobalScale = 1.2f;
    ImGui::StyleColorsDark();
    ImGui_ImplWin32_Init(hwnd_);
}

void MainWindow::cleanup_imgui() {
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
}

void MainWindow::render_ui() {
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
    ImGui::Begin("ShadowKey", nullptr,
                 ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                 ImGuiWindowFlags_NoMove | ImGuiWindowFlags_MenuBar);

    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Load Script...")) on_load_script();
            if (ImGui::MenuItem("Save Script")) on_save_script();
            ImGui::Separator();
            if (ImGui::MenuItem("Exit")) PostQuitMessage(0);
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Help")) {
            ImGui::MenuItem("Hotkeys:");
            ImGui::MenuItem("  Ctrl+Alt+R  Toggle Recording");
            ImGui::MenuItem("  Ctrl+Alt+S  Stop Playback");
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }

    if (ImGui::BeginTabBar("MainTabs")) {
        if (ImGui::BeginTabItem("Main")) {
            render_main_tab();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Script Editor")) {
            render_script_editor();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Settings")) {
            render_settings_tab();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Log")) {
            render_log_tab();
            ImGui::EndTabItem();
        }
    }
    ImGui::EndTabBar();

    ImGui::End();
    ImGui::Render();
}

void MainWindow::render_main_tab() {
    ImGui::Text("Status: %s", executor_state_str(executor_.state()).c_str());
    if (is_recording_) ImGui::Text("Recording... (%zu events)", recorded_events_.size());

    ImGui::Separator();
    ImGui::Text("Script: %s", current_script_path_.empty() ? "(none)" : current_script_path_.c_str());
    ImGui::Text("Actions: %zu", executor_.current_script().actions.size());

    ImGui::Separator();
    ImGui::Spacing();

    if (is_recording_) {
        if (ImGui::Button("Stop Recording", ImVec2(150, 0))) {
            on_stop_recording();
        }
    } else {
        if (ImGui::Button("Start Recording", ImVec2(150, 0))) {
            on_start_recording();
        }
    }
    ImGui::SameLine();

    auto state = executor_.state();
    if (state == ExecutorState::Running) {
        if (ImGui::Button("Pause", ImVec2(100, 0))) executor_.pause();
        ImGui::SameLine();
        if (ImGui::Button("Stop", ImVec2(100, 0))) executor_.stop();
    } else if (state == ExecutorState::Paused) {
        if (ImGui::Button("Resume", ImVec2(100, 0))) executor_.resume();
        ImGui::SameLine();
        if (ImGui::Button("Stop", ImVec2(100, 0))) executor_.stop();
    } else {
        if (ImGui::Button("Play", ImVec2(100, 0))) {
            on_start_playback();
        }
    }

    ImGui::SameLine();
    bool filter = anti_config_.record_filter_mousemove;
    ImGui::Checkbox("Filter MouseMove", &filter);
    anti_config_.record_filter_mousemove = filter;
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Skip mouse move events during recording to reduce script size");

    ImGui::Spacing();
    ImGui::Separator();

    ImGui::Text("Recorded Events (%zu):", recorded_events_.size());
    if (!recorded_events_.empty()) {
        int n = std::min(20, static_cast<int>(recorded_events_.size()));
        if (ImGui::BeginChild("EventList", ImVec2(0, 200), true)) {
            for (int i = recorded_events_.size() - n; i < static_cast<int>(recorded_events_.size()); ++i) {
                const auto& ev = recorded_events_[i];
                std::string desc;
                switch (ev.type) {
                    case InputEventType::KeyDown: desc = "KeyDown vk=" + std::to_string(ev.key.vk_code); break;
                    case InputEventType::KeyUp: desc = "KeyUp vk=" + std::to_string(ev.key.vk_code); break;
                    case InputEventType::MouseMove: desc = "MouseMove (" + std::to_string(ev.mouse_move.x) + "," + std::to_string(ev.mouse_move.y) + ")"; break;
                    case InputEventType::MouseLeftDown: desc = "LeftDown (" + std::to_string(ev.mouse_click.x) + "," + std::to_string(ev.mouse_click.y) + ")"; break;
                    case InputEventType::MouseLeftUp: desc = "LeftUp"; break;
                    case InputEventType::MouseRightDown: desc = "RightDown"; break;
                    case InputEventType::MouseRightUp: desc = "RightUp"; break;
                    case InputEventType::MouseWheel: desc = "Wheel delta=" + std::to_string(ev.wheel.delta); break;
                }
                ImGui::Text("%d: %s", i, desc.c_str());
            }
        }
        ImGui::EndChild();
    }
}

void MainWindow::render_script_editor() {
    auto& script = const_cast<Script&>(executor_.current_script());

    ImGui::InputText("Name", script_name_buffer_, sizeof(script_name_buffer_));
    ImGui::InputText("Description", script_desc_buffer_, sizeof(script_desc_buffer_));

    ImGui::SliderFloat("Speed Multiplier", &speed_multiplier_, 0.1f, 5.0f, "%.1fx");
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("2.0x = twice as fast, 0.5x = half speed");
    executor_.set_speed_multiplier(speed_multiplier_);

    int loop = static_cast<int>(script.loop_count);
    ImGui::InputInt("Loop Count", &loop);
    if (loop < 0) loop = 0;
    if (loop > 9999) loop = 9999;
    script.loop_count = static_cast<uint32_t>(loop);

    ImGui::Separator();
    ImGui::Text("Actions (%zu):", script.actions.size());
    ImGui::SameLine();
    if (ImGui::SmallButton("Apply Changes")) {
        script.name = script_name_buffer_;
        script.description = script_desc_buffer_;
        script.speed_multiplier = speed_multiplier_;
        executor_.set_speed_multiplier(speed_multiplier_);
        add_log("Script changes applied");
    }

    if (ImGui::BeginChild("ActionList", ImVec2(0, 0), true)) {
        for (int i = 0; i < static_cast<int>(script.actions.size()); ++i) {
            auto& action = script.actions[i];
            std::string label = std::to_string(i) + ": " + event_desc(action);

            ImGui::PushID(i);
            ImGui::Text("%s", label.c_str());

            ImGui::SameLine(350);
            ImGui::PushItemWidth(80);
            int delay = static_cast<int>(action.delay_after_ms);
            ImGui::InputInt("ms", &delay);
            if (delay < 0) delay = 0;
            action.delay_after_ms = static_cast<uint32_t>(delay);
            ImGui::PopItemWidth();

            ImGui::SameLine();
            if (ImGui::SmallButton("X")) {
                delete_action_idx_ = i;
            }

            if (delete_action_idx_ == i) {
                ImGui::SameLine();
                ImGui::Text("Confirm?");
                ImGui::SameLine();
                if (ImGui::SmallButton("Yes")) {
                    script.actions.erase(script.actions.begin() + i);
                    delete_action_idx_ = -1;
                    add_log("Action " + std::to_string(i) + " deleted");
                }
                ImGui::SameLine();
                if (ImGui::SmallButton("No")) {
                    delete_action_idx_ = -1;
                }
            }

            ImGui::PopID();
        }
    }
    ImGui::EndChild();
}

void MainWindow::render_settings_tab() {
    ImGui::Text("Anti-Detection Settings");

    int delay_min = anti_config_.min_delay_ms;
    int delay_max = anti_config_.max_delay_ms;
    int offset = anti_config_.click_offset_px;
    int steps_min = anti_config_.mouse_move_steps_min;
    int steps_max = anti_config_.mouse_move_steps_max;
    int step_delay = anti_config_.mouse_step_delay_ms;
    int ss_interval = anti_config_.screenshot_interval_ms;
    int typing_min = anti_config_.typing_min_delay_ms;
    int typing_max = anti_config_.typing_max_delay_ms;

    ImGui::Text("Operation Delay");
    ImGui::SliderInt("Min Delay (ms)", &delay_min, 10, 500);
    ImGui::SliderInt("Max Delay (ms)", &delay_max, 20, 1000);

    ImGui::Separator();
    ImGui::Text("Mouse Behavior");
    ImGui::SliderInt("Click Offset (px)", &offset, 0, 20);
    ImGui::SliderInt("Mouse Steps Min", &steps_min, 1, 20);
    ImGui::SliderInt("Mouse Steps Max", &steps_max, 1, 30);
    ImGui::SliderInt("Step Delay (ms)", &step_delay, 1, 50);

    const char* curve_items[] = { "Linear", "Bezier" };
    int curve_idx = static_cast<int>(anti_config_.curve_mode);
    ImGui::Combo("Mouse Curve", &curve_idx, curve_items, IM_ARRAYSIZE(curve_items));
    anti_config_.curve_mode = static_cast<MouseCurveMode>(curve_idx);

    ImGui::Separator();
    ImGui::Text("Typing Simulation");
    ImGui::SliderInt("Typing Min Delay (ms)", &typing_min, 10, 300);
    ImGui::SliderInt("Typing Max Delay (ms)", &typing_max, 20, 500);

    ImGui::Separator();
    ImGui::Text("Image Matching");
    ImGui::SliderInt("Screenshot Interval (ms)", &ss_interval, 100, 3000);

    anti_config_.min_delay_ms = delay_min;
    anti_config_.max_delay_ms = delay_max;
    anti_config_.click_offset_px = offset;
    anti_config_.mouse_move_steps_min = steps_min;
    anti_config_.mouse_move_steps_max = steps_max;
    anti_config_.mouse_step_delay_ms = step_delay;
    anti_config_.screenshot_interval_ms = ss_interval;
    anti_config_.typing_min_delay_ms = typing_min;
    anti_config_.typing_max_delay_ms = typing_max;
}

void MainWindow::render_log_tab() {
    std::lock_guard<std::mutex> lock(log_mutex);
    ImGui::TextUnformatted(status_log.c_str());
    if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
        ImGui::SetScrollHereY(1.0f);
    }
}

void MainWindow::on_start_recording() {
    recorded_events_.clear();
    is_recording_ = true;
    add_log("Recording started... (Ctrl+Alt+R to stop)");

    hook_.start([this](const InputEvent& ev) {
        recording_callback(ev);
    });
}

void MainWindow::on_stop_recording() {
    is_recording_ = false;
    hook_.stop();
    add_log("Recording stopped. " + std::to_string(recorded_events_.size()) + " events captured.");
}

void MainWindow::on_save_script() {
    auto& script = const_cast<Script&>(executor_.current_script());

    if (recorded_events_.empty() && script.actions.empty()) {
        add_log("No events or actions to save.");
        return;
    }

    OPENFILENAMEA ofn = {};
    char path[MAX_PATH] = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd_;
    ofn.lpstrFilter = "ShadowKey Scripts (*.sks)\0*.sks\0All Files (*.*)\0*.*\0";
    ofn.lpstrFile = path;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrDefExt = "sks";
    ofn.Flags = OFN_HIDEREADONLY;

    if (!recorded_events_.empty()) {
        Script new_script;
        new_script.name = script_name_buffer_[0] ? script_name_buffer_ : "Recorded " + std::to_string(time(nullptr));
        new_script.description = script_desc_buffer_;
        new_script.loop_count = 1;
        new_script.speed_multiplier = speed_multiplier_;
        new_script.created_at = static_cast<uint32_t>(time(nullptr));

        for (const auto& ev : recorded_events_) {
            ScriptAction action{};
            action.type = ev.type;
            switch (ev.type) {
                case InputEventType::KeyDown:
                case InputEventType::KeyUp:
                    action.key.vk_code = ev.key.vk_code;
                    break;
                case InputEventType::MouseMove:
                    action.mouse_move.x = ev.mouse_move.x;
                    action.mouse_move.y = ev.mouse_move.y;
                    break;
                case InputEventType::MouseLeftDown:
                case InputEventType::MouseLeftUp:
                case InputEventType::MouseRightDown:
                case InputEventType::MouseRightUp:
                    action.mouse_click.x = ev.mouse_click.x;
                    action.mouse_click.y = ev.mouse_click.y;
                    break;
                case InputEventType::MouseWheel:
                    action.wheel.delta = ev.wheel.delta;
                    break;
            }
            new_script.actions.push_back(action);
        }

        if (GetSaveFileNameA(&ofn)) {
            current_script_path_ = path;
            if (ScriptCodec::save(new_script, current_script_path_)) {
                executor_.load_script(new_script);
                add_log("Script saved: " + current_script_path_);
            }
        }
    } else if (!script.actions.empty()) {
        script.name = script_name_buffer_;
        script.description = script_desc_buffer_;
        script.speed_multiplier = speed_multiplier_;

        if (GetSaveFileNameA(&ofn)) {
            current_script_path_ = path;
            if (ScriptCodec::save(script, current_script_path_)) {
                add_log("Script saved: " + current_script_path_);
            }
        }
    }
}

void MainWindow::on_load_script() {
    OPENFILENAMEA ofn = {};
    char path[MAX_PATH] = {};

    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd_;
    ofn.lpstrFilter = "ShadowKey Scripts (*.sks)\0*.sks\0All Files (*.*)\0*.*\0";
    ofn.lpstrFile = path;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;

    if (GetOpenFileNameA(&ofn)) {
        current_script_path_ = path;
        if (executor_.load(path)) {
            const auto& s = executor_.current_script();
            strncpy_s(script_name_buffer_, s.name.c_str(), sizeof(script_name_buffer_) - 1);
            strncpy_s(script_desc_buffer_, s.description.c_str(), sizeof(script_desc_buffer_) - 1);
            speed_multiplier_ = static_cast<float>(s.speed_multiplier);
            add_log("Script loaded: " + current_script_path_);
        } else {
            add_log("Failed to load script!");
        }
    }
}

void MainWindow::on_start_playback() {
    if (executor_.current_script().actions.empty()) {
        add_log("No script loaded!");
        return;
    }
    if (!executor_.start()) {
        add_log("Failed to start playback!");
    }
}

void MainWindow::on_stop_playback() {
    executor_.stop();
    add_log("Playback stopped.");
}

void MainWindow::recording_callback(const InputEvent& event) {
    if (anti_config_.record_filter_mousemove && event.type == InputEventType::MouseMove)
        return;

    std::lock_guard<std::mutex> lock(events_mutex_);
    pending_events_.push_back(event);
}

void MainWindow::executor_status_callback(ExecutorState state, int index, const std::string& msg) {
    add_log("[" + executor_state_str(state) + "] " + msg);
}
