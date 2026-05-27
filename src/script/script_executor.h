#pragma once
#include "script_format.h"
#include "core/input_sim.h"
#include "core/image_matcher.h"
#include "core/anti_detect.h"
#include "utils/timer.h"
#include <atomic>
#include <functional>
#include <string>

enum class ExecutorState {
    Idle,
    Running,
    Paused,
    Stopped,
    Error
};

class ScriptExecutor {
public:
    ScriptExecutor();

    bool load(const std::string& path);
    bool load_script(const Script& script);
    bool start();
    void stop();
    void pause();
    void resume();

    ExecutorState state() const;
    int current_action_index() const;
    const Script& current_script() const;

    using StatusCallback = std::function<void(ExecutorState, int, const std::string&)>;
    void set_status_callback(StatusCallback cb);

private:
    Script script_;
    std::atomic<ExecutorState> state_;
    std::atomic<int> action_index_;
    StatusCallback callback_;

    void execute_loop();
    void execute_action(const ScriptAction& action);
    bool execute_image_trigger(const ScriptAction& action);
    void emit_status(ExecutorState state, int index, const std::string& msg);
};
