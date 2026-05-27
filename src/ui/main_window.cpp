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

MainWindow::MainWindow()
    : hwnd_(nullptr), hinstance_(nullptr),
      is_recording_(false),
      anti_config_(AntiDetect::config()) {}

MainWindow::~MainWindow() {
    hook_.stop();
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

    RECT rect = {0, 0, 600, 500};
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
            ImGui::MenuItem("About", nullptr, false, false);
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }

    if (ImGui::BeginTabBar("MainTabs")) {
        if (ImGui::BeginTabItem("Main")) {
            render_main_tab();
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

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Text("Recorded Events:");
    if (!recorded_events_.empty()) {
        int n = std::min(20, static_cast<int>(recorded_events_.size()));
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

    ImGui::SliderInt("Min Delay (ms)", &delay_min, 10, 500);
    ImGui::SliderInt("Max Delay (ms)", &delay_max, 20, 1000);
    ImGui::SliderInt("Click Offset (px)", &offset, 0, 20);
    ImGui::SliderInt("Mouse Steps Min", &steps_min, 1, 20);
    ImGui::SliderInt("Mouse Steps Max", &steps_max, 1, 30);
    ImGui::SliderInt("Step Delay (ms)", &step_delay, 1, 50);
    ImGui::SliderInt("Screenshot Interval (ms)", &ss_interval, 100, 3000);

    anti_config_.min_delay_ms = delay_min;
    anti_config_.max_delay_ms = delay_max;
    anti_config_.click_offset_px = offset;
    anti_config_.mouse_move_steps_min = steps_min;
    anti_config_.mouse_move_steps_max = steps_max;
    anti_config_.mouse_step_delay_ms = step_delay;
    anti_config_.screenshot_interval_ms = ss_interval;
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
    add_log("Recording started...");

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
    if (recorded_events_.empty()) {
        add_log("No events to save.");
        return;
    }

    Script script;
    script.name = "Recorded " + std::to_string(time(nullptr));
    script.description = "Recorded by ShadowKey";
    script.loop_count = 1;
    script.created_at = static_cast<uint32_t>(time(nullptr));

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
        script.actions.push_back(action);
    }

    current_script_path_ = script.name + ".sks";
    if (ScriptCodec::save(script, current_script_path_)) {
        add_log("Script saved: " + current_script_path_);
    } else {
        add_log("Failed to save script!");
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
    recorded_events_.push_back(event);
}

void MainWindow::executor_status_callback(ExecutorState state, int index, const std::string& msg) {
    add_log("[" + executor_state_str(state) + "] " + msg);
}
