#pragma once

#include "raylib.h"
#include <string>
#include <unordered_map>

namespace IronHull
{
    struct InputAction
    {
        KeyboardKey key;
        MouseButton mouse;
        GamepadButton gamepad_button;
        GamepadAxis gamepad_axis;
    };

    class Input
    {
        private:
            std::unordered_map<std::string, InputAction> actions;
        private:
            static Input& get_singleton();
        public:
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
