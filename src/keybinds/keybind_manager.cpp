#include "keybinds/keybind_manager.h"
#include "utils/logging.h"
#include <algorithm>
#include <fstream>
#include <ostream>
#include <sstream>

// --- Constructor and Initialization ---
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

// --- Keyboard Input Handling ---
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

// --- Joystick Input Handling ---
void KeybindManager::setJoystickButton(const std::string& action, int joystickId, uint8_t button) {
    auto it = keybinds_.find(action);
    if (it != keybinds_.end()) {
        it->second.input = JoystickInput{joystickId, button};
        LOG_DEBUG("Set joystick button for " << action << " to JOY_" << joystickId << "_BUTTON_" << static_cast<int>(button));
    } else {
        LOG_DEBUG("Cannot set joystick button: Action not found: " << action);
    }
}

void KeybindManager::setJoystickHat(const std::string& action, int joystickId, uint8_t hat, uint8_t direction) {
    auto it = keybinds_.find(action);
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
        LOG_DEBUG("Set joystick hat for " << action << " to JOY_" << joystickId << "_HAT_" << static_cast<int>(hat) << "_" << directionStr);
    } else {
        LOG_DEBUG("Cannot set joystick hat: Action not found: " << action);
    }
}

void KeybindManager::setJoystickAxis(const std::string& action, int joystickId, uint8_t axis, bool positiveDirection) {
    auto it = keybinds_.find(action);
    if (it != keybinds_.end()) {
        it->second.input = JoystickAxisInput{joystickId, axis, positiveDirection};
        LOG_DEBUG("Set joystick axis for " << action << " to JOY_" << joystickId << "_AXIS_" << static_cast<int>(axis) << "_" << (positiveDirection ? "POSITIVE" : "NEGATIVE"));
    } else {
        LOG_DEBUG("Cannot set joystick axis: Action not found: " << action);
    }
}

// --- Action and Tooltip Management ---
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

// --- Load and Save Keybinds ---
void KeybindManager::loadKeybinds(const std::map<std::string, std::string>& keybindData) {
    for (const auto& [key, value] : keybindData) {
        auto it = keybinds_.find(key);
        if (it != keybinds_.end()) {
            // Check if the value is a joystick button (e.g., "JOY_0_BUTTON_3")
            if (value.find("JOY_") == 0) {
                if (value.find("_BUTTON_") != std::string::npos) {
                    size_t joyEnd = value.find("_BUTTON_");
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
                }
                // Check if the value is a joystick hat (e.g., "JOY_0_HAT_0_LEFT")
                else if (value.find("_HAT_") != std::string::npos) {
                    size_t joyEnd = value.find("_HAT_");
                    size_t hatEnd = value.find("_", joyEnd + 5);
                    if (hatEnd != std::string::npos) {
                        std::string joyIdStr = value.substr(4, joyEnd - 4);
                        std::string hatStr = value.substr(joyEnd + 5, hatEnd - (joyEnd + 5));
                        std::string directionStr = value.substr(hatEnd + 1);
                        try {
                            int joystickId = std::stoi(joyIdStr);
                            uint8_t hat = static_cast<uint8_t>(std::stoi(hatStr));
                            uint8_t direction = SDL_HAT_CENTERED;
                            if (directionStr == "UP") direction = SDL_HAT_UP;
                            else if (directionStr == "DOWN") direction = SDL_HAT_DOWN;
                            else if (directionStr == "LEFT") direction = SDL_HAT_LEFT;
                            else if (directionStr == "RIGHT") direction = SDL_HAT_RIGHT;
                            it->second.input = JoystickHatInput{joystickId, hat, direction};
                            LOG_DEBUG("Loaded joystick hat keybind " << key << " = " << value);
                        } catch (...) {
                            LOG_DEBUG("Invalid joystick hat keybind format for " << key << ": " << value << ", keeping default");
                        }
                    }
                }
                // Check if the value is a joystick axis (e.g., "JOY_0_AXIS_0_POSITIVE")
                else if (value.find("_AXIS_") != std::string::npos) {
                    size_t joyEnd = value.find("_AXIS_");
                    size_t axisEnd = value.find("_", joyEnd + 6);
                    if (axisEnd != std::string::npos) {
                        std::string joyIdStr = value.substr(4, joyEnd - 4);
                        std::string axisStr = value.substr(joyEnd + 6, axisEnd - (joyEnd + 6));
                        std::string directionStr = value.substr(axisEnd + 1);
                        try {
                            int joystickId = std::stoi(joyIdStr);
                            uint8_t axis = static_cast<uint8_t>(std::stoi(axisStr));
                            bool positiveDirection = (directionStr == "POSITIVE");
                            it->second.input = JoystickAxisInput{joystickId, axis, positiveDirection};
                            LOG_DEBUG("Loaded joystick axis keybind " << key << " = " << value);
                        } catch (...) {
                            LOG_DEBUG("Invalid joystick axis keybind format for " << key << ": " << value << ", keeping default");
                        }
                    }
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
            file << action << "=" << ss.str() << "\n";
            LOG_DEBUG("Saved joystick hat keybind " << action << " = " << ss.str());
        } else if (std::holds_alternative<JoystickAxisInput>(keybind.input)) {
            const auto& axisInput = std::get<JoystickAxisInput>(keybind.input);
            std::stringstream ss;
            ss << "JOY_" << axisInput.joystickId << "_AXIS_" << static_cast<int>(axisInput.axis) << "_"
               << (axisInput.positiveDirection ? "POSITIVE" : "NEGATIVE");
            file << action << "=" << ss.str() << "\n";
            LOG_DEBUG("Saved joystick axis keybind " << action << " = " << ss.str());
        }
    }
}

// --- Joystick Event Matching ---
bool KeybindManager::isJoystickAction(const SDL_JoyButtonEvent& event, const std::string& action) const {
    auto it = keybinds_.find(action);
    if (it != keybinds_.end() && std::holds_alternative<JoystickInput>(it->second.input)) {
        const auto& joyInput = std::get<JoystickInput>(it->second.input);
        return joyInput.joystickId == event.which && joyInput.button == event.button;
    }
    return false;
}

bool KeybindManager::isJoystickHatAction(const SDL_JoyHatEvent& event, const std::string& action) const {
    auto it = keybinds_.find(action);
    if (it != keybinds_.end() && std::holds_alternative<JoystickHatInput>(it->second.input)) {
        const auto& hatInput = std::get<JoystickHatInput>(it->second.input);
        return hatInput.joystickId == event.which && hatInput.hat == event.hat && hatInput.direction == event.value;
    }
    return false;
}

bool KeybindManager::isJoystickAxisAction(const SDL_JoyAxisEvent& event, const std::string& action) const {
    auto it = keybinds_.find(action);
    if (it != keybinds_.end() && std::holds_alternative<JoystickAxisInput>(it->second.input)) {
        const auto& axisInput = std::get<JoystickAxisInput>(it->second.input);
        if (axisInput.joystickId == event.which && axisInput.axis == event.axis) {
            // Consider the axis triggered if the value exceeds a threshold (e.g., 50% of max)
            const int threshold = 16384; // SDL joystick axis range is -32768 to 32767
            if (axisInput.positiveDirection && event.value > threshold) {
                return true;
            } else if (!axisInput.positiveDirection && event.value < -threshold) {
                return true;
            }
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
            std::string sdlKeyName = std::string(keyName);
            if (sdlKeyName.substr(0, 5) == "SDLK_") {
                sdlKeyName = sdlKeyName.substr(5);
            }
            std::transform(sdlKeyName.begin(), sdlKeyName.end(), sdlKeyName.begin(), ::toupper);
            return sdlKeyName;
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
            }
            return ss.str();
        }
    } else if (event.type == SDL_JOYAXISMOTION) {
        const int threshold = 16384;
        if (event.jaxis.value > threshold || event.jaxis.value < -threshold) {
            bool positiveDirection = (event.jaxis.value > 0);
            std::stringstream ss;
            ss << "JOY_" << event.jaxis.which << "_AXIS_" << static_cast<int>(event.jaxis.axis) << "_"
               << (positiveDirection ? "POSITIVE" : "NEGATIVE");
            return ss.str();
        }
    }
    return "";
}

bool KeybindManager::isAction(const SDL_KeyboardEvent& event, const std::string& action) const {
    SDL_Keycode key = getKey(action);
    bool match = (event.keysym.sym == key); // Simplified: event.keysym.sym is the correct member
    LOG_DEBUG("Checking action: " << action << ", key: " << SDL_GetKeyName(event.keysym.sym) 
              << " (keycode: " << event.keysym.sym << ") against " << SDL_GetKeyName(key) 
              << " (keycode: " << key << "), Match=" << match);
    return match;
}