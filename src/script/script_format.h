#pragma once
#include "core/input_hook.h"
#include <string>
#include <vector>
#include <cstdint>

/// A single scriptable action.
struct ScriptAction {
    InputEventType type;
    uint32_t       delay_after_ms{0};

    // -- Event data ---------------------------------------------------------
    union {
        struct { uint32_t vk_code; }          key;         ///< For KeyDown / KeyUp.
        struct { int x; int y; }              mouse_move;  ///< For MouseMove.
        struct { int x; int y; }              mouse_click; ///< For click events.
        struct { int delta; }                 wheel;       ///< For MouseWheel.
    };

    // -- Text typing -------------------------------------------------------
    struct {
        std::string text;
        int         min_delay_ms = 30;
        int         max_delay_ms = 120;
    } typing;

    // -- Image trigger -----------------------------------------------------
    struct {
        std::string image_template;
        double      match_threshold  = 0.8;
        bool        wait_for_match   = false;
        int         timeout_ms       = 5000;
        int         click_offset_x   = 0;
        int         click_offset_y   = 0;
    } image_trigger;
};

/// A complete .sks script.
struct Script {
    std::string              name;
    std::string              description;
    uint32_t                 loop_count{1};
    uint32_t                 created_at{0};
    double                   speed_multiplier{1.0};
    std::vector<ScriptAction> actions;
};

/// .sks JSON serialisation / deserialisation.
class ScriptCodec {
public:
    ScriptCodec() = delete;

    /// Serialise to a JSON file.
    [[nodiscard]] static bool save(const Script& script, const std::string& path);

    /// Deserialise from a JSON file.
    [[nodiscard]] static Script load(const std::string& path);

    /// Serialise to a JSON string.
    [[nodiscard]] static std::string to_json(const Script& script);

    /// Deserialise from a JSON string.
    [[nodiscard]] static Script from_json(const std::string& json_str);
};
