#pragma once
#include "script_format.h"
#include "core/input_sim.h"
#include "core/image_matcher.h"
#include "core/anti_detect.h"
#include "utils/timer.h"
#include <atomic>
#include <functional>
#include <string>

/// Life-cycle states of the script executor.
enum class ExecutorState {
    Idle,
    Running,
    Paused,
    Stopped,
    Error
};

/// Drives script playback in a background thread.
class ScriptExecutor {
public:
    ScriptExecutor() noexcept;

    ScriptExecutor(const ScriptExecutor&)            = delete;
    ScriptExecutor& operator=(const ScriptExecutor&) = delete;

    // -- Life-cycle ---------------------------------------------------------

    /// Load a script from a .sks file.
    [[nodiscard]] bool load(const std::string& path);

    /// Load a script object directly.
    [[nodiscard]] bool load_script(const Script& script);

    /// Begin execution in a detached thread.
    [[nodiscard]] bool start();
    void stop() noexcept;
    void pause() noexcept;
    void resume() noexcept;

    // -- Runtime queries ----------------------------------------------------

    [[nodiscard]] ExecutorState state() const noexcept { return state_.load(); }
    [[nodiscard]] int           current_action_index() const noexcept { return action_index_.load(); }
    [[nodiscard]] const Script& current_script() const noexcept { return script_; }

    // -- Runtime configuration ----------------------------------------------

    /// Change speed multiplier while playing (0.1 – 5.0).
    void set_speed_multiplier(double multiplier) noexcept;

    // -- Callback -----------------------------------------------------------

    using StatusCallback = std::function<void(ExecutorState, int, const std::string&)>;
    void set_status_callback(StatusCallback cb) noexcept { callback_ = std::move(cb); }

private:
    Script                   script_;
    std::atomic<ExecutorState> state_{ExecutorState::Idle};
    std::atomic<int>          action_index_{-1};
    std::atomic<double>       speed_multiplier_{1.0};
    StatusCallback            callback_;

    void execute_loop();
    void execute_action(const ScriptAction& action);
    bool execute_image_trigger(const ScriptAction& action);
    void emit_status(ExecutorState state, int index, const std::string& msg);
};
