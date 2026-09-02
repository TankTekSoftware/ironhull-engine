#include <IronHull/input/Input.hpp>

#include <algorithm>

namespace IronHull
{
    namespace
    {
        constexpr int GAMEPAD_ID = 0;
        constexpr float GAMEPAD_AXIS_DEADZONE = 0.5f;

        InputAction& get_or_create_action(std::unordered_map<std::string, InputAction>& actions, const std::string& action)
        {
            return actions[action];
        }

        template <typename T>
        void add_binding(std::vector<T>& bindings, T value)
        {
            if (std::find(bindings.begin(), bindings.end(), value) == bindings.end()) {
                bindings.push_back(value);
            }
        }

        bool axis_engaged(GamepadAxis axis)
        {
            if (axis < 0 || !IsGamepadAvailable(GAMEPAD_ID)) {
                return false;
            }

            float movement = GetGamepadAxisMovement(GAMEPAD_ID, axis);
            return movement > GAMEPAD_AXIS_DEADZONE || movement < -GAMEPAD_AXIS_DEADZONE;
        }
    }

    Input& Input::get_singleton()
    {
        static Input singleton;
        return singleton;
    }

    void Input::register_key_action(const std::string &action, KeyboardKey keyboard_key)
    {
        Input& self = Input::get_singleton();
        add_binding(get_or_create_action(self.actions, action).keys, keyboard_key);
    }

    void Input::register_mouse_action(const std::string &action, MouseButton mouse_button)
    {
        Input& self = Input::get_singleton();
        add_binding(get_or_create_action(self.actions, action).mouse_buttons, mouse_button);
    }

    void Input::register_joybutton_action(const std::string &action, GamepadButton gamepad_button)
    {
        Input& self = Input::get_singleton();
        add_binding(get_or_create_action(self.actions, action).gamepad_buttons, gamepad_button);
    }

    void Input::register_joyaxis_action(const std::string &action, GamepadAxis gamepad_axis)
    {
        Input& self = Input::get_singleton();
        add_binding(get_or_create_action(self.actions, action).gamepad_axes, gamepad_axis);
    }

    bool Input::is_action_pressed(const std::string &action)
    {
        Input& self = Input::get_singleton();

        auto it = self.actions.find(action);
        if (it == self.actions.end()) {
            return false;
        }

        const InputAction& binding = it->second;

        for (KeyboardKey key : binding.keys) {
            if (IsKeyDown(key)) {
                return true;
            }
        }

        for (MouseButton mouse : binding.mouse_buttons) {
            if (IsMouseButtonDown(mouse)) {
                return true;
            }
        }

        if (IsGamepadAvailable(GAMEPAD_ID)) {
            for (GamepadButton gamepad_button : binding.gamepad_buttons) {
                if (IsGamepadButtonDown(GAMEPAD_ID, gamepad_button)) {
                    return true;
                }
            }
        }

        for (GamepadAxis axis : binding.gamepad_axes) {
            if (axis_engaged(axis)) {
                return true;
            }
        }

        return false;
    }

    bool Input::is_action_released(const std::string &action)
    {
        Input& self = Input::get_singleton();

        auto it = self.actions.find(action);
        if (it == self.actions.end()) {
            return false;
        }

        const InputAction& binding = it->second;

        for (KeyboardKey key : binding.keys) {
            if (IsKeyReleased(key)) {
                return true;
            }
        }

        for (MouseButton mouse : binding.mouse_buttons) {
            if (IsMouseButtonReleased(mouse)) {
                return true;
            }
        }

        if (IsGamepadAvailable(GAMEPAD_ID)) {
            for (GamepadButton gamepad_button : binding.gamepad_buttons) {
                if (IsGamepadButtonReleased(GAMEPAD_ID, gamepad_button)) {
                    return true;
                }
            }
        }

        return false;
    }

    bool Input::is_action_just_pressed(const std::string &action)
    {
        Input& self = Input::get_singleton();

        auto it = self.actions.find(action);
        if (it == self.actions.end()) {
            return false;
        }

        const InputAction& binding = it->second;

        for (KeyboardKey key : binding.keys) {
            if (IsKeyPressed(key)) {
                return true;
            }
        }

        for (MouseButton mouse : binding.mouse_buttons) {
            if (IsMouseButtonPressed(mouse)) {
                return true;
            }
        }

        if (IsGamepadAvailable(GAMEPAD_ID)) {
            for (GamepadButton gamepad_button : binding.gamepad_buttons) {
                if (IsGamepadButtonPressed(GAMEPAD_ID, gamepad_button)) {
                    return true;
                }
            }
        }

        return false;
    }
}
