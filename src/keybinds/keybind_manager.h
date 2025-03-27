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

    // New method for joystick support
    void setJoystickButton(const std::string& action, int joystickId, uint8_t button);

    // Methods for loading/saving keybindings
    void loadKeybinds(const std::map<std::string, std::string>& keybindData);
    void saveKeybinds(std::ofstream& file) const;

    // Check if an action is bound to a joystick button and matches the event
    bool isJoystickAction(const SDL_JoyButtonEvent& event, const std::string& action) const;

private:
    struct JoystickInput {
        int joystickId;
        uint8_t button;
    };

    struct Keybind {
        std::variant<SDL_Keycode, JoystickInput> input; // Can be either a key or a joystick button
        std::string tooltip;
    };

    std::map<std::string, Keybind> keybinds_;

    void initializeDefaults();
};

#endif // KEYBIND_MANAGER_H