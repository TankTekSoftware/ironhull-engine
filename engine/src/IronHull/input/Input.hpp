#pragma once

#include "raylib.h"
#include <string>
#include <unordered_map>
#include <vector>

namespace IronHull
{
    struct InputAction
    {
        std::vector<KeyboardKey> keys;
        std::vector<MouseButton> mouse_buttons;
        std::vector<GamepadButton> gamepad_buttons;
        std::vector<GamepadAxis> gamepad_axes;
    };

    class Input
    {
        private:
            std::unordered_map<std::string, InputAction> actions;
        private:
            static Input& get_singleton();
        public:
            // Each of these appends a binding to the action, so an action can
            // respond to multiple keys/buttons/axes at once (e.g. W and Up
            // both bound to "forward").
            static void register_key_action(const std::string& action, KeyboardKey keyboard_key);
            static void register_mouse_action(const std::string& action, MouseButton mouse_button);
            static void register_joybutton_action(const std::string& action, GamepadButton gamepad_button);
            static void register_joyaxis_action(const std::string& action, GamepadAxis gamepad_axis);
        public:
            static bool is_action_pressed(const std::string& action);
            static bool is_action_released(const std::string& action);
            static bool is_action_just_pressed(const std::string& action);
    };
}
