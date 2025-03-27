#ifndef KEYBIND_MANAGER_H
#define KEYBIND_MANAGER_H

#include "keybinds/ikeybind_provider.h"
#include <map>
#include <string>
#include <variant>
#include <SDL2/SDL.h>

class KeybindManager : public IKeybindProvider {
public:
    KeybindManager();
    ~KeybindManager() override = default;

    // IKeybindProvider interface
    SDL_Keycode getKey(const std::string& action) const override;
    void setKey(const std::string& action, SDL_Keycode key) override;
    std::vector<std::string> getActions() const override;
    std::string getTooltip(const std::string& action) const override;
    void setJoystickButton(const std::string& action, int joystickId, uint8_t button) override;
    void setJoystickHat(const std::string& action, int joystickId, uint8_t hat, uint8_t direction) override;
    void setJoystickAxis(const std::string& action, int joystickId, uint8_t axis, bool positiveDirection) override;
    std::string eventToString(const SDL_Event& event) const override; // New method

    // Methods for loading/saving keybindings
    void loadKeybinds(const std::map<std::string, std::string>& keybindData);
    void saveKeybinds(std::ofstream& file) const;

    // Check if an action is bound to a keyboard or joystick input and matches the event
    bool isAction(const SDL_KeyboardEvent& event, const std::string& action) const; // Renamed from InputManager::isAction
    bool isJoystickAction(const SDL_JoyButtonEvent& event, const std::string& action) const;
    bool isJoystickHatAction(const SDL_JoyHatEvent& event, const std::string& action) const;
    bool isJoystickAxisAction(const SDL_JoyAxisEvent& event, const std::string& action) const;

private:
    struct JoystickInput {
        int joystickId;
        uint8_t button;
    };

    struct JoystickHatInput {
        int joystickId;
        uint8_t hat;
        uint8_t direction;
    };

    struct JoystickAxisInput {
        int joystickId;
        uint8_t axis;
        bool positiveDirection;
    };

    struct Keybind {
        std::variant<SDL_Keycode, JoystickInput, JoystickHatInput, JoystickAxisInput> input;
        std::string tooltip;
    };

    std::map<std::string, Keybind> keybinds_;

    void initializeDefaults();
};

#endif // KEYBIND_MANAGER_H