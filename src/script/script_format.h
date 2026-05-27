#pragma once
#include "core/input_hook.h"
#include <string>
#include <vector>
#include <cstdint>

struct ScriptAction {
    InputEventType type;
    uint32_t delay_after_ms;

    union {
        struct { uint32_t vk_code; } key;
        struct { int x; int y; } mouse_move;
        struct { int x; int y; } mouse_click;
        struct { int delta; } wheel;
    };

    struct {
        std::string text;
        int min_delay_ms = 30;
        int max_delay_ms = 120;
    } typing;

    struct {
        std::string image_template;
        double match_threshold = 0.8;
        bool wait_for_match = false;
        int timeout_ms = 5000;
        int click_offset_x = 0;
        int click_offset_y = 0;
    } image_trigger;
};

struct Script {
    std::string name;
    std::string description;
    uint32_t loop_count = 1;
    uint32_t created_at = 0;
    double speed_multiplier = 1.0;
    std::vector<ScriptAction> actions;
};

class ScriptCodec {
public:
    static bool save(const Script& script, const std::string& path);
    static Script load(const std::string& path);

    static std::string to_json(const Script& script);
    static Script from_json(const std::string& json_str);
};
