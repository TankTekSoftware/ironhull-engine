#include <IronHull/input/Input.hpp>

namespace IronHull
{
    namespace
    {
        constexpr int GAMEPAD_ID = 0;
        constexpr float GAMEPAD_AXIS_DEADZONE = 0.5f;

        // A freshly registered action starts with every binding "unset" so that
        // registering one input type (e.g. a key) doesn't accidentally make the
        // action also respond to gamepad button 0, mouse button 0, gamepad axis 0, etc.
        InputAction make_unbound_action()
        {
            InputAction action{};
            action.key = KEY_NULL;
            action.mouse = static_cast<MouseButton>(-1);
            action.gamepad_button = GAMEPAD_BUTTON_UNKNOWN;
            action.gamepad_axis = static_cast<GamepadAxis>(-1);
            return action;
        }

        InputAction& get_or_create_action(std::unordered_map<std::string, InputAction>& actions, const std::string& action)
        {
            auto it = actions.find(action);
            if (it == actions.end()) {
                it = actions.emplace(action, make_unbound_action()).first;
            }
            return it->second;
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
        get_or_create_action(self.actions, action).key = keyboard_key;
    }

    void Input::register_mouse_action(const std::string &action, MouseButton mouse_button)
    {
        Input& self = Input::get_singleton();
        get_or_create_action(self.actions, action).mouse = mouse_button;
    }

    void Input::register_joybutton_action(const std::string &action, GamepadButton gamepad_button)
    {
        Input& self = Input::get_singleton();
        get_or_create_action(self.actions, action).gamepad_button = gamepad_button;
    }

    void Input::register_joyaxis_action(const std::string &action, GamepadAxis gamepad_axis)
    {
        Input& self = Input::get_singleton();
        get_or_create_action(self.actions, action).gamepad_axis = gamepad_axis;
    }

    bool Input::is_action_pressed(const std::string &action)
    {
        Input& self = Input::get_singleton();

        auto it = self.actions.find(action);
        if (it == self.actions.end()) {
            return false;
        }

        const InputAction& binding = it->second;

        if (binding.key != KEY_NULL && IsKeyDown(binding.key)) {
            return true;
        }

        if (binding.mouse >= 0 && IsMouseButtonDown(binding.mouse)) {
            return true;
        }

        if (binding.gamepad_button != GAMEPAD_BUTTON_UNKNOWN && IsGamepadAvailable(GAMEPAD_ID)
            && IsGamepadButtonDown(GAMEPAD_ID, binding.gamepad_button)) {
            return true;
        }

        if (axis_engaged(binding.gamepad_axis)) {
            return true;
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

        if (binding.key != KEY_NULL && IsKeyReleased(binding.key)) {
            return true;
        }

        if (binding.mouse >= 0 && IsMouseButtonReleased(binding.mouse)) {
            return true;
        }

        if (binding.gamepad_button != GAMEPAD_BUTTON_UNKNOWN && IsGamepadAvailable(GAMEPAD_ID)
            && IsGamepadButtonReleased(GAMEPAD_ID, binding.gamepad_button)) {
            return true;
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

        if (binding.key != KEY_NULL && IsKeyPressed(binding.key)) {
            return true;
        }

        if (binding.mouse >= 0 && IsMouseButtonPressed(binding.mouse)) {
            return true;
        }

        if (binding.gamepad_button != GAMEPAD_BUTTON_UNKNOWN && IsGamepadAvailable(GAMEPAD_ID)
            && IsGamepadButtonPressed(GAMEPAD_ID, binding.gamepad_button)) {
            return true;
        }

        return false;
    }
}
