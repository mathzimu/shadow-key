#include "script_format.h"
#include "utils/logger.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <sstream>

using json = nlohmann::json;

static const char* event_type_name(InputEventType t) {
    switch (t) {
        case InputEventType::KeyDown: return "key_down";
        case InputEventType::KeyUp: return "key_up";
        case InputEventType::MouseMove: return "mouse_move";
        case InputEventType::MouseLeftDown: return "mouse_left_down";
        case InputEventType::MouseLeftUp: return "mouse_left_up";
        case InputEventType::MouseRightDown: return "mouse_right_down";
        case InputEventType::MouseRightUp: return "mouse_right_up";
        case InputEventType::MouseWheel: return "mouse_wheel";
        default: return "unknown";
    }
}

static InputEventType event_type_from_name(const std::string& name) {
    if (name == "key_down") return InputEventType::KeyDown;
    if (name == "key_up") return InputEventType::KeyUp;
    if (name == "mouse_move") return InputEventType::MouseMove;
    if (name == "mouse_left_down") return InputEventType::MouseLeftDown;
    if (name == "mouse_left_up") return InputEventType::MouseLeftUp;
    if (name == "mouse_right_down") return InputEventType::MouseRightDown;
    if (name == "mouse_right_up") return InputEventType::MouseRightUp;
    if (name == "mouse_wheel") return InputEventType::MouseWheel;
    return InputEventType::KeyDown;
}

bool ScriptCodec::save(const Script& script, const std::string& path) {
    std::ofstream file(path);
    if (!file.is_open()) {
        LOG_ERROR("Failed to open {} for writing", path);
        return false;
    }
    file << to_json(script);
    LOG_INFO("Script saved to {}", path);
    return true;
}

Script ScriptCodec::load(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        LOG_ERROR("Failed to open {} for reading", path);
        return {};
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return from_json(buffer.str());
}

std::string ScriptCodec::to_json(const Script& script) {
    json j;
    j["name"] = script.name;
    j["description"] = script.description;
    j["loop_count"] = script.loop_count;
    j["created_at"] = script.created_at;

    json actions = json::array();
    for (const auto& action : script.actions) {
        json a;
        a["type"] = event_type_name(action.type);
        a["delay_after_ms"] = action.delay_after_ms;

        switch (action.type) {
            case InputEventType::KeyDown:
            case InputEventType::KeyUp:
                a["vk_code"] = action.key.vk_code;
                break;
            case InputEventType::MouseMove:
                a["x"] = action.mouse_move.x;
                a["y"] = action.mouse_move.y;
                break;
            case InputEventType::MouseLeftDown:
            case InputEventType::MouseLeftUp:
            case InputEventType::MouseRightDown:
            case InputEventType::MouseRightUp:
                a["x"] = action.mouse_click.x;
                a["y"] = action.mouse_click.y;
                break;
            case InputEventType::MouseWheel:
                a["delta"] = action.wheel.delta;
                break;
        }

        if (!action.image_trigger.image_template.empty()) {
            a["image_trigger"] = {
                {"template", action.image_trigger.image_template},
                {"threshold", action.image_trigger.match_threshold},
                {"wait_for_match", action.image_trigger.wait_for_match},
                {"timeout_ms", action.image_trigger.timeout_ms},
                {"click_offset_x", action.image_trigger.click_offset_x},
                {"click_offset_y", action.image_trigger.click_offset_y}
            };
        }

        actions.push_back(a);
    }
    j["actions"] = actions;

    return j.dump(2);
}

Script ScriptCodec::from_json(const std::string& json_str) {
    Script script;
    try {
        json j = json::parse(json_str);

        script.name = j.value("name", "Untitled");
        script.description = j.value("description", "");
        script.loop_count = j.value("loop_count", 1);
        script.created_at = j.value("created_at", 0);

        if (j.contains("actions")) {
            for (const auto& a : j["actions"]) {
                ScriptAction action{};
                action.type = event_type_from_name(a["type"]);
                action.delay_after_ms = a.value("delay_after_ms", 0);

                switch (action.type) {
                    case InputEventType::KeyDown:
                    case InputEventType::KeyUp:
                        action.key.vk_code = a["vk_code"];
                        break;
                    case InputEventType::MouseMove:
                        action.mouse_move.x = a["x"];
                        action.mouse_move.y = a["y"];
                        break;
                    case InputEventType::MouseLeftDown:
                    case InputEventType::MouseLeftUp:
                    case InputEventType::MouseRightDown:
                    case InputEventType::MouseRightUp:
                        action.mouse_click.x = a["x"];
                        action.mouse_click.y = a["y"];
                        break;
                    case InputEventType::MouseWheel:
                        action.wheel.delta = a["delta"];
                        break;
                }

                if (a.contains("image_trigger")) {
                    const auto& img = a["image_trigger"];
                    action.image_trigger.image_template = img.value("template", "");
                    action.image_trigger.match_threshold = img.value("threshold", 0.8);
                    action.image_trigger.wait_for_match = img.value("wait_for_match", false);
                    action.image_trigger.timeout_ms = img.value("timeout_ms", 5000);
                    action.image_trigger.click_offset_x = img.value("click_offset_x", 0);
                    action.image_trigger.click_offset_y = img.value("click_offset_y", 0);
                }

                script.actions.push_back(action);
            }
        }
    } catch (const std::exception& e) {
        LOG_ERROR("JSON parse error: {}", e.what());
    }

    return script;
}
