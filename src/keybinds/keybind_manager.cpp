#include "keybinds/keybind_manager.h"
#include "utils/string_utils.h"
#include "log/logging.h"
#include <algorithm>
#include <sstream>

KeybindManager::KeybindManager() {
    initializeDefaults();
}

void KeybindManager::initializeDefaults() {
    if (!keybinds_.empty()) {
        LOG_DEBUG("KeybindManager: Skipping initializeDefaults; keybinds already loaded.");
        return;
    }
    keybinds_[("Previous Table")] = {SDL_GetKeyFromName("Left Shift")};
    keybinds_[("Next Table")] = {SDL_GetKeyFromName("Right Shift")};
    keybinds_[("Fast Previous Table")] = {SDL_GetKeyFromName("Left Ctrl")};
    keybinds_[("Fast Next Table")] = {SDL_GetKeyFromName("Right Ctrl")};
    keybinds_[("Jump Next Letter")] = {SDL_GetKeyFromName("/")};
    keybinds_[("Jump Previous Letter")] = {SDL_GetKeyFromName("Z")};
    keybinds_[("Random Table")] = {SDL_GetKeyFromName("R")};
    keybinds_[("Launch Table")] = {SDL_GetKeyFromName("Return")};
    keybinds_[("Toggle Config")] = {SDL_GetKeyFromName("C")};
    keybinds_[("Quit")] = {SDL_GetKeyFromName("Q")};
    keybinds_[("Screenshot Mode")] = {SDL_GetKeyFromName("S")};
    keybinds_[("Screenshot Key")] = {SDL_GetKeyFromName("S")};
    keybinds_[("Screenshot Quit")] = {SDL_GetKeyFromName("Q")};
    keybinds_[("Toggle Editor")] = {SDL_GetKeyFromName("M")};
    keybinds_[("Toggle Catalog")] = {SDL_GetKeyFromName("N")};
}

std::string KeybindManager::getActionForKey(const std::string& key) const {
    SDL_Keycode keyCode = SDL_GetKeyFromName(key.c_str());
    if (keyCode == SDLK_UNKNOWN) {
        LOG_DEBUG("KeybindManager: Invalid key name: " << key);
        return "";
    }
    for (const auto& [action, keybind] : keybinds_) {
        if (std::holds_alternative<SDL_Keycode>(keybind.input)) {
            if (std::get<SDL_Keycode>(keybind.input) == keyCode) {
                LOG_DEBUG("KeybindManager: Matched action " << action << " for key " << key);
                return action;
            }
        }
    }
    LOG_DEBUG("KeybindManager: No action for key " << key);
    return "";
}

// std::string KeybindManager::normalizeAction(const std::string& action) const {
//     std::string normalized = action;
//     normalized.erase(std::remove(normalized.begin(), normalized.end(), ' '), normalized.end());
//     for (size_t i = 1; i < normalized.size(); ++i) {
//         if (std::islower(normalized[i]) && (i == 1 || normalized[i-1] != ' ')) {
//             normalized[i] = std::toupper(normalized[i]);
//         }
//     }
//     if (!normalized.empty()) {
//         normalized[0] = std::toupper(normalized[0]);
//     }
//     return normalized;
// }

SDL_Keycode KeybindManager::getKey(const std::string& action) const {
    auto it = keybinds_.find((action));
    if (it != keybinds_.end() && std::holds_alternative<SDL_Keycode>(it->second.input)) {
        return std::get<SDL_Keycode>(it->second.input);
    }
    LOG_DEBUG("KeybindManager: Keybind not found or not a keyboard input for action: " << action);
    return SDLK_UNKNOWN;
}

void KeybindManager::setKey(const std::string& action, SDL_Keycode key) {
    std::string normalizedAction = (action);
    auto it = keybinds_.find(normalizedAction);
    if (it != keybinds_.end()) {
        it->second.input = key;
        LOG_DEBUG("KeybindManager: Set key for " << normalizedAction << " to " << SDL_GetKeyName(key));
    } else {
        keybinds_[normalizedAction] = {key};
        LOG_DEBUG("KeybindManager: Created new keybind for " << normalizedAction << " with key " << SDL_GetKeyName(key));
    }
}

void KeybindManager::setJoystickButton(const std::string& action, int joystickId, uint8_t button) {
    std::string normalizedAction = (action);
    auto it = keybinds_.find(normalizedAction);
    if (it != keybinds_.end()) {
        it->second.input = JoystickInput{joystickId, button};
        LOG_DEBUG("KeybindManager: Set joystick button for " << normalizedAction << " to JOY_" << joystickId << "_BUTTON_" << static_cast<int>(button));
    } else {
        keybinds_[normalizedAction] = {JoystickInput{joystickId, button}};
        LOG_DEBUG("KeybindManager: Created new joystick button keybind for " << normalizedAction);
    }
}

void KeybindManager::setJoystickHat(const std::string& action, int joystickId, uint8_t hat, uint8_t direction) {
    std::string normalizedAction = (action);
    auto it = keybinds_.find(normalizedAction);
    if (it != keybinds_.end()) {
        it->second.input = JoystickHatInput{joystickId, hat, direction};
        std::string directionStr;
        switch (direction) {
            case SDL_HAT_UP: directionStr = "UP"; break;
            case SDL_HAT_DOWN: directionStr = "DOWN"; break;
            case SDL_HAT_LEFT: directionStr = "LEFT"; break;
            case SDL_HAT_RIGHT: directionStr = "RIGHT"; break;
            default: directionStr = "UNKNOWN"; break;
        }
        LOG_DEBUG("KeybindManager: Set joystick hat for " << normalizedAction << " to JOY_" << joystickId << "_HAT_" << static_cast<int>(hat) << "_" << directionStr);
    } else {
        keybinds_[normalizedAction] = {JoystickHatInput{joystickId, hat, direction}};
        LOG_DEBUG("KeybindManager: Created new joystick hat keybind for " << normalizedAction);
    }
}

void KeybindManager::setJoystickAxis(const std::string& action, int joystickId, uint8_t axis, bool positiveDirection) {
    std::string normalizedAction = (action);
    auto it = keybinds_.find(normalizedAction);
    if (it != keybinds_.end()) {
        it->second.input = JoystickAxisInput{joystickId, axis, positiveDirection};
        LOG_DEBUG("KeybindManager: Set joystick axis for " << normalizedAction << " to JOY_" << joystickId << "_AXIS_" << static_cast<int>(axis) << "_" << (positiveDirection ? "POSITIVE" : "NEGATIVE"));
    } else {
        keybinds_[normalizedAction] = {JoystickAxisInput{joystickId, axis, positiveDirection}};
        LOG_DEBUG("KeybindManager: Created new joystick axis keybind for " << normalizedAction);
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

bool KeybindManager::isAction(const SDL_KeyboardEvent& event, const std::string& action) const {
    std::string normalizedAction = (action);
    auto it = keybinds_.find(normalizedAction);
    if (it != keybinds_.end() && std::holds_alternative<SDL_Keycode>(it->second.input)) {
        SDL_Keycode key = std::get<SDL_Keycode>(it->second.input);
        // Use SDL_GetKeyFromScancode to handle scancode-based events
        SDL_Keycode eventKey = SDL_GetKeyFromScancode(event.keysym.scancode);
        bool match = (eventKey == key && event.state == SDL_PRESSED);
        LOG_DEBUG("KeybindManager: Checking action: " << normalizedAction << ", key: " << SDL_GetKeyName(eventKey) 
                  << " (keycode: " << eventKey << ", scancode: " << event.keysym.scancode << ") against " 
                  << SDL_GetKeyName(key) << " (keycode: " << key << "), Match=" << match);
        return match;
    }
    LOG_DEBUG("KeybindManager: No keybind for action: " << normalizedAction);
    return false;
}

bool KeybindManager::isJoystickAction(const SDL_JoyButtonEvent& event, const std::string& action) const {
    std::string normalizedAction = (action);
    auto it = keybinds_.find(normalizedAction);
    if (it != keybinds_.end() && std::holds_alternative<JoystickInput>(it->second.input)) {
        const auto& joyInput = std::get<JoystickInput>(it->second.input);
        return joyInput.joystickId == event.which && joyInput.button == event.button && event.state == SDL_PRESSED;
    }
    return false;
}

bool KeybindManager::isJoystickHatAction(const SDL_JoyHatEvent& event, const std::string& action) const {
    std::string normalizedAction = (action);
    auto it = keybinds_.find(normalizedAction);
    if (it != keybinds_.end() && std::holds_alternative<JoystickHatInput>(it->second.input)) {
        const auto& hatInput = std::get<JoystickHatInput>(it->second.input);
        return hatInput.joystickId == event.which && hatInput.hat == event.hat && hatInput.direction == event.value;
    }
    return false;
}

bool KeybindManager::isJoystickAxisAction(const SDL_JoyAxisEvent& event, const std::string& action) const {
    std::string normalizedAction = (action);
    auto it = keybinds_.find(normalizedAction);
    if (it != keybinds_.end() && std::holds_alternative<JoystickAxisInput>(it->second.input)) {
        const auto& axisInput = std::get<JoystickAxisInput>(it->second.input);
        if (axisInput.joystickId == event.which && axisInput.axis == event.axis) {
            const int threshold = 16384; // SDL joystick axis range is -32768 to 32767
            return (axisInput.positiveDirection && event.value > threshold) ||
                   (!axisInput.positiveDirection && event.value < -threshold);
        }
    }
    return false;
}

std::string KeybindManager::eventToString(const SDL_Event& event) const {
    if (event.type == SDL_KEYDOWN) {
        SDL_Keycode keyCode = event.key.keysym.sym;
        if (keyCode == SDLK_ESCAPE || keyCode == SDLK_UNKNOWN) {
            return "";
        }
        const char* keyName = SDL_GetKeyName(keyCode);
        if (keyName && *keyName) {
            return std::string(keyName);
        }
    } else if (event.type == SDL_JOYBUTTONDOWN) {
        std::stringstream ss;
        ss << "JOY_" << event.jbutton.which << "_BUTTON_" << static_cast<int>(event.jbutton.button);
        return ss.str();
    } else if (event.type == SDL_JOYHATMOTION) {
        if (event.jhat.value == SDL_HAT_UP || event.jhat.value == SDL_HAT_DOWN ||
            event.jhat.value == SDL_HAT_LEFT || event.jhat.value == SDL_HAT_RIGHT) {
            std::stringstream ss;
            ss << "JOY_" << event.jhat.which << "_HAT_" << static_cast<int>(event.jhat.hat) << "_";
            switch (event.jhat.value) {
                case SDL_HAT_UP: ss << "UP"; break;
                case SDL_HAT_DOWN: ss << "DOWN"; break;
                case SDL_HAT_LEFT: ss << "LEFT"; break;
                case SDL_HAT_RIGHT: ss << "RIGHT"; break;
                default: return "";
            }
            return ss.str();
        }
    } else if (event.type == SDL_JOYAXISMOTION) {
        const int threshold = 16384;
        if (std::abs(event.jaxis.value) > threshold) {
            std::stringstream ss;
            ss << "JOY_" << event.jaxis.which << "_AXIS_" << static_cast<int>(event.jaxis.axis) << "_"
               << (event.jaxis.value > 0 ? "POSITIVE" : "NEGATIVE");
            return ss.str();
        }
    }
    return "";
}

void KeybindManager::loadKeybinds(const std::map<std::string, std::string>& keybindData) {
    for (const auto& [key, value] : keybindData) {
        std::string normalizedKey = (key);
        if (value.find("JOY_") == 0) {
            try {
                if (value.find("_BUTTON_") != std::string::npos) {
                    size_t joyEnd = value.find("_BUTTON_");
                    int joystickId = std::stoi(value.substr(4, joyEnd - 4));
                    uint8_t button = static_cast<uint8_t>(std::stoi(value.substr(joyEnd + 8)));
                    setJoystickButton(normalizedKey, joystickId, button);
                } else if (value.find("_HAT_") != std::string::npos) {
                    size_t joyEnd = value.find("_HAT_");
                    size_t hatEnd = value.find("_", joyEnd + 5);
                    if (hatEnd != std::string::npos) {
                        int joystickId = std::stoi(value.substr(4, joyEnd - 4));
                        uint8_t hat = static_cast<uint8_t>(std::stoi(value.substr(joyEnd + 5, hatEnd - (joyEnd + 5))));
                        std::string directionStr = value.substr(hatEnd + 1);
                        uint8_t direction = SDL_HAT_CENTERED;
                        if (directionStr == "UP") direction = SDL_HAT_UP;
                        else if (directionStr == "DOWN") direction = SDL_HAT_DOWN;
                        else if (directionStr == "LEFT") direction = SDL_HAT_LEFT;
                        else if (directionStr == "RIGHT") direction = SDL_HAT_RIGHT;
                        setJoystickHat(normalizedKey, joystickId, hat, direction);
                    }
                } else if (value.find("_AXIS_") != std::string::npos) {
                    size_t joyEnd = value.find("_AXIS_");
                    size_t axisEnd = value.find("_", joyEnd + 6);
                    if (axisEnd != std::string::npos) {
                        int joystickId = std::stoi(value.substr(4, joyEnd - 4));
                        uint8_t axis = static_cast<uint8_t>(std::stoi(value.substr(joyEnd + 6, axisEnd - (joyEnd + 6))));
                        bool positiveDirection = (value.substr(axisEnd + 1) == "POSITIVE");
                        setJoystickAxis(normalizedKey, joystickId, axis, positiveDirection);
                    }
                }
            } catch (const std::exception& e) {
                LOG_ERROR("KeybindManager: Invalid joystick keybind format for " << normalizedKey << ": " << value << ", keeping default. Error: " << e.what());
            }
        } else {
            SDL_Keycode keyCode = SDL_GetKeyFromName(value.c_str());
            if (keyCode != SDLK_UNKNOWN) {
                setKey(normalizedKey, keyCode);
                LOG_DEBUG("KeybindManager: Set key for " << normalizedKey << " to " << value);
            } else {
                LOG_ERROR("KeybindManager: Invalid key name for " << normalizedKey << ": " << value << ", keeping default");
            }
        }
    }
}


void KeybindManager::saveKeybinds(std::map<std::string, std::string>& keybinds) const {
    keybinds.clear();
    for (const auto& [action, keybind] : keybinds_) {
        if (std::holds_alternative<SDL_Keycode>(keybind.input)) {
            SDL_Keycode key = std::get<SDL_Keycode>(keybind.input);
            const char* keyName = SDL_GetKeyName(key);
            if (keyName && *keyName) {
                keybinds[action] = keyName; // Use SDL long name directly
            } else {
                keybinds[action] = "Unknown";
            }
        } else if (std::holds_alternative<JoystickInput>(keybind.input)) {
            const auto& joyInput = std::get<JoystickInput>(keybind.input);
            std::stringstream ss;
            ss << "JOY_" << joyInput.joystickId << "_BUTTON_" << static_cast<int>(joyInput.button);
            keybinds[action] = ss.str();
        } else if (std::holds_alternative<JoystickHatInput>(keybind.input)) {
            const auto& hatInput = std::get<JoystickHatInput>(keybind.input);
            std::stringstream ss;
            ss << "JOY_" << hatInput.joystickId << "_HAT_" << static_cast<int>(hatInput.hat) << "_";
            switch (hatInput.direction) {
                case SDL_HAT_UP: ss << "UP"; break;
                case SDL_HAT_DOWN: ss << "DOWN"; break;
                case SDL_HAT_LEFT: ss << "LEFT"; break;
                case SDL_HAT_RIGHT: ss << "RIGHT"; break;
                default: ss << "UNKNOWN"; break;
            }
            keybinds[action] = ss.str();
        } else if (std::holds_alternative<JoystickAxisInput>(keybind.input)) {
            const auto& axisInput = std::get<JoystickAxisInput>(keybind.input);
            std::stringstream ss;
            ss << "JOY_" << axisInput.joystickId << "_AXIS_" << static_cast<int>(axisInput.axis) << "_"
               << (axisInput.positiveDirection ? "POSITIVE" : "NEGATIVE");
            keybinds[action] = ss.str();
        }
    }
}