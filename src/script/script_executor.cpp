#include "script_executor.h"
#include "core/screen_capture.h"
#include "utils/logger.h"
#include <thread>
#include <opencv2/opencv.hpp>

ScriptExecutor::ScriptExecutor() noexcept
    : state_(ExecutorState::Idle), action_index_(-1), speed_multiplier_(1.0) {}

bool ScriptExecutor::load(const std::string& path) {
    script_ = ScriptCodec::load(path);
    if (script_.actions.empty()) {
        LOG_ERROR("Loaded script has no actions: {}", path);
        return false;
    }
    LOG_INFO("Script '{}' loaded with {} actions", script_.name, script_.actions.size());
    return true;
}

bool ScriptExecutor::load_script(const Script& script) {
    script_ = script;
    if (script_.actions.empty()) {
        LOG_ERROR("Script has no actions");
        return false;
    }
    return true;
}

bool ScriptExecutor::start() {
    if (state_ != ExecutorState::Idle && state_ != ExecutorState::Stopped) {
        LOG_WARN("Executor already running or paused");
        return false;
    }
    state_ = ExecutorState::Running;
    action_index_ = 0;
    emit_status(ExecutorState::Running, 0, "Execution started");
    std::thread(&ScriptExecutor::execute_loop, this).detach();
    return true;
}

void ScriptExecutor::stop() {
    state_ = ExecutorState::Stopped;
    emit_status(ExecutorState::Stopped, action_index_, "Execution stopped");
}

void ScriptExecutor::pause() {
    if (state_ == ExecutorState::Running) {
        state_ = ExecutorState::Paused;
        emit_status(ExecutorState::Paused, action_index_, "Execution paused");
    }
}

void ScriptExecutor::resume() {
    if (state_ == ExecutorState::Paused) {
        state_ = ExecutorState::Running;
        emit_status(ExecutorState::Running, action_index_, "Execution resumed");
    }
}

ExecutorState ScriptExecutor::state() const {
    return state_;
}

int ScriptExecutor::current_action_index() const {
    return action_index_;
}

const Script& ScriptExecutor::current_script() const {
    return script_;
}

void ScriptExecutor::set_speed_multiplier(double multiplier) noexcept {
    speed_multiplier_.store(std::max(0.1, multiplier));
}

void ScriptExecutor::execute_loop() {
    speed_multiplier_.store(std::max(0.1, script_.speed_multiplier));

    for (uint32_t loop = 0; loop < script_.loop_count || script_.loop_count == 0; ++loop) {
        if (state_ != ExecutorState::Running) break;

        for (size_t i = 0; i < script_.actions.size(); ++i) {
            if (state_ != ExecutorState::Running) break;

            while (state_ == ExecutorState::Paused) {
                Sleep(10);
                if (state_ == ExecutorState::Stopped) return;
            }

            action_index_ = static_cast<int>(i);
            emit_status(ExecutorState::Running, action_index_,
                        "Executing action " + std::to_string(i + 1));

            execute_action(script_.actions[i]);

            double speed = speed_multiplier_.load();
            int delay = static_cast<int>(AntiDetect::random_delay() / speed);
            Sleep(delay);
        }
    }

    state_ = ExecutorState::Idle;
    action_index_ = -1;
    emit_status(ExecutorState::Idle, -1, "Execution completed");
}

void ScriptExecutor::execute_action(const ScriptAction& action) {
    if (!action.typing.text.empty()) {
        TypingConfig cfg;
        cfg.min_delay_ms = action.typing.min_delay_ms;
        cfg.max_delay_ms = action.typing.max_delay_ms;
        InputSim::type_text(action.typing.text, cfg);
        return;
    }

    if (!action.image_trigger.image_template.empty()) {
        bool ok = execute_image_trigger(action);
        if (!ok) {
            LOG_WARN("Image trigger failed for '{}', skipping action",
                     action.image_trigger.image_template);
            return;
        }
    }

    switch (action.type) {
        case InputEventType::KeyDown:
            InputSim::key_down(action.key.vk_code);
            break;
        case InputEventType::KeyUp:
            InputSim::key_up(action.key.vk_code);
            break;
        case InputEventType::MouseMove:
            InputSim::mouse_move(action.mouse_move.x, action.mouse_move.y);
            break;
        case InputEventType::MouseLeftDown:
            InputSim::mouse_left_click(
                action.mouse_click.x + AntiDetect::random_offset(),
                action.mouse_click.y + AntiDetect::random_offset());
            break;
        case InputEventType::MouseLeftUp:
            InputSim::mouse_left_up();
            break;
        case InputEventType::MouseRightDown:
            InputSim::mouse_right_click(
                action.mouse_click.x + AntiDetect::random_offset(),
                action.mouse_click.y + AntiDetect::random_offset());
            break;
        case InputEventType::MouseRightUp:
            break;
        case InputEventType::MouseWheel:
            InputSim::mouse_scroll(action.wheel.delta);
            break;
    }
}

bool ScriptExecutor::execute_image_trigger(const ScriptAction& action) {
    cv::Mat templ = cv::imread(action.image_trigger.image_template);
    if (templ.empty()) {
        LOG_ERROR("Failed to load template image: {}", action.image_trigger.image_template);
        return false;
    }

    MatchConfig config;
    config.threshold = action.image_trigger.match_threshold;

    Timer timer;
    while (true) {
        if (state_ != ExecutorState::Running) return false;

        auto results = ImageMatcher::find_template_on_screen(templ, config);
        if (!results.empty()) {
            auto& best = results[0];
            int cx = best.location.x + best.template_width / 2 + action.image_trigger.click_offset_x;
            int cy = best.location.y + best.template_height / 2 + action.image_trigger.click_offset_y;
            InputSim::mouse_left_click(cx, cy);
            LOG_INFO("Image matched '{}' at ({},{}), confidence={:.2f}",
                     action.image_trigger.image_template, cx, cy, best.confidence);
            return true;
        }

        if (action.image_trigger.wait_for_match) {
            if (timer.elapsed_ms() >= action.image_trigger.timeout_ms) {
                LOG_WARN("Image trigger timeout for '{}'", action.image_trigger.image_template);
                return false;
            }
            Sleep(AntiDetect::config().screenshot_interval_ms);
        } else {
            return false;
        }
    }
}

void ScriptExecutor::emit_status(ExecutorState state, int index, const std::string& msg) {
    if (callback_) {
        callback_(state, index, msg);
    }
}
