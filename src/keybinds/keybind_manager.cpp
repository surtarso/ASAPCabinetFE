#include "keybinds/keybind_manager.h"
#include "utils/logging.h"
#include <algorithm>
#include <fstream>
#include <ostream>
#include <sstream>

KeybindManager::KeybindManager() {
    initializeDefaults();
}

void KeybindManager::initializeDefaults() {
    keybinds_["PreviousTable"] = {SDLK_LSHIFT, "Key to select the previous table in the list."};
    keybinds_["NextTable"] = {SDLK_RSHIFT, "Key to select the next table in the list."};
    keybinds_["FastPrevTable"] = {SDLK_LCTRL, "Key to quickly jump back 10 tables."};
    keybinds_["FastNextTable"] = {SDLK_RCTRL, "Key to quickly jump forward 10 tables."};
    keybinds_["JumpNextLetter"] = {SDLK_SLASH, "Key to jump to the next table starting with a different letter."};
    keybinds_["JumpPrevLetter"] = {SDLK_z, "Key to jump to the previous table starting with a different letter."};
    keybinds_["RandomTable"] = {SDLK_r, "Key to jump to a random table."};
    keybinds_["LaunchTable"] = {SDLK_RETURN, "Key to launch the selected table."};
    keybinds_["ToggleConfig"] = {SDLK_c, "Key to open or close the configuration menu."};
    keybinds_["Quit"] = {SDLK_q, "Key to exit menus and application."};
    keybinds_["ConfigSave"] = {SDLK_SPACE, "Key to save changes in the configuration menu."};
    keybinds_["ConfigClose"] = {SDLK_q, "Key to close the configuration menu without saving."};
    keybinds_["ScreenshotMode"] = {SDLK_s, "Key to launch a table in screenshot mode."};
    keybinds_["ScreenshotKey"] = {SDLK_s, "Key to take a screenshot while in screenshot mode."};
    keybinds_["ScreenshotQuit"] = {SDLK_q, "Key to quit screenshot mode."};
}

SDL_Keycode KeybindManager::getKey(const std::string& action) const {
    auto it = keybinds_.find(action);
    if (it != keybinds_.end() && std::holds_alternative<SDL_Keycode>(it->second.input)) {
        return std::get<SDL_Keycode>(it->second.input);
    }
    LOG_DEBUG("Keybind not found or not a keyboard input for action: " << action);
    return SDLK_UNKNOWN;
}

void KeybindManager::setKey(const std::string& action, SDL_Keycode key) {
    auto it = keybinds_.find(action);
    if (it != keybinds_.end()) {
        it->second.input = key;
        LOG_DEBUG("Set key for " << action << " to " << SDL_GetKeyName(key));
    } else {
        LOG_DEBUG("Cannot set key: Action not found: " << action);
    }
}

void KeybindManager::setJoystickButton(const std::string& action, int joystickId, uint8_t button) {
    auto it = keybinds_.find(action);
    if (it != keybinds_.end()) {
        it->second.input = JoystickInput{joystickId, button};
        LOG_DEBUG("Set joystick button for " << action << " to JOY_" << joystickId << "_BUTTON_" << static_cast<int>(button));
    } else {
        LOG_DEBUG("Cannot set joystick button: Action not found: " << action);
    }
}

std::vector<std::string> KeybindManager::getActions() const {
    std::vector<std::string> actions;
    for (const auto& [action, _] : keybinds_) {
        actions.push_back(action);
    }
    std::sort(actions.begin(), actions.end());
    return actions;
}

std::string KeybindManager::getTooltip(const std::string& action) const {
    auto it = keybinds_.find(action);
    if (it != keybinds_.end()) {
        return it->second.tooltip;
    }
    return "";
}

void KeybindManager::loadKeybinds(const std::map<std::string, std::string>& keybindData) {
    for (const auto& [key, value] : keybindData) {
        auto it = keybinds_.find(key);
        if (it != keybinds_.end()) {
            // Check if the value is a joystick input (e.g., "JOY_0_BUTTON_3")
            if (value.find("JOY_") == 0) {
                size_t joyEnd = value.find("_BUTTON_");
                if (joyEnd != std::string::npos) {
                    std::string joyIdStr = value.substr(4, joyEnd - 4);
                    std::string buttonStr = value.substr(joyEnd + 8);
                    try {
                        int joystickId = std::stoi(joyIdStr);
                        uint8_t button = static_cast<uint8_t>(std::stoi(buttonStr));
                        it->second.input = JoystickInput{joystickId, button};
                        LOG_DEBUG("Loaded joystick keybind " << key << " = " << value);
                    } catch (...) {
                        LOG_DEBUG("Invalid joystick keybind format for " << key << ": " << value << ", keeping default");
                    }
                } else {
                    LOG_DEBUG("Invalid joystick keybind format for " << key << ": " << value << ", keeping default");
                }
            } else {
                // Assume it's a keyboard input
                SDL_Keycode keyCode = SDL_GetKeyFromName(value.c_str());
                if (keyCode != SDLK_UNKNOWN) {
                    it->second.input = keyCode;
                    LOG_DEBUG("Loaded keybind " << key << " = " << value << " (keycode: " << keyCode << ")");
                } else {
                    LOG_DEBUG("Invalid key name for " << key << ": " << value << ", keeping default");
                }
            }
        }
    }
}

void KeybindManager::saveKeybinds(std::ofstream& file) const {
    if (!file.is_open()) {
        LOG_DEBUG("Cannot save keybinds: File stream is not open");
        return;
    }
    file << "[Keybinds]\n";
    for (const auto& action : getActions()) {
        const auto& keybind = keybinds_.at(action);
        if (std::holds_alternative<SDL_Keycode>(keybind.input)) {
            SDL_Keycode key = std::get<SDL_Keycode>(keybind.input);
            const char* keyName = SDL_GetKeyName(key);
            file << action << "=" << (keyName ? keyName : "Unknown") << "\n";
            LOG_DEBUG("Saved keybind " << action << " = " << (keyName ? keyName : "Unknown"));
        } else if (std::holds_alternative<JoystickInput>(keybind.input)) {
            const auto& joyInput = std::get<JoystickInput>(keybind.input);
            std::stringstream ss;
            ss << "JOY_" << joyInput.joystickId << "_BUTTON_" << static_cast<int>(joyInput.button);
            file << action << "=" << ss.str() << "\n";
            LOG_DEBUG("Saved joystick keybind " << action << " = " << ss.str());
        }
    }
}

bool KeybindManager::isJoystickAction(const SDL_JoyButtonEvent& event, const std::string& action) const {
    auto it = keybinds_.find(action);
    if (it != keybinds_.end() && std::holds_alternative<JoystickInput>(it->second.input)) {
        const auto& joyInput = std::get<JoystickInput>(it->second.input);
        return joyInput.joystickId == event.which && joyInput.button == event.button;
    }
    return false;
}