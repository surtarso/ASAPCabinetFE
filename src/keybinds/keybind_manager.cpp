#include "keybinds/keybind_manager.h"
#include "utils/logging.h"
#include <algorithm>
#include <fstream> // Explicitly include for std::ofstream
#include <ostream> // Ensure operator<< overloads are available

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
    if (it != keybinds_.end()) {
        return it->second.key;
    }
    LOG_DEBUG("Keybind not found for action: " << action);
    return SDLK_UNKNOWN;
}

void KeybindManager::setKey(const std::string& action, SDL_Keycode key) {
    auto it = keybinds_.find(action);
    if (it != keybinds_.end()) {
        it->second.key = key;
        LOG_DEBUG("Set key for " << action << " to " << SDL_GetKeyName(key));
    } else {
        LOG_DEBUG("Cannot set key: Action not found: " << action);
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
            SDL_Keycode keyCode = SDL_GetKeyFromName(value.c_str());
            if (keyCode != SDLK_UNKNOWN) {
                it->second.key = keyCode;
                LOG_DEBUG("Loaded keybind " << key << " = " << value << " (keycode: " << keyCode << ")");
            } else {
                LOG_DEBUG("Invalid key name for " << key << ": " << value << ", keeping default");
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
        SDL_Keycode key = getKey(action);
        const char* keyName = SDL_GetKeyName(key);
        file << action << "=" << (keyName ? keyName : "Unknown") << "\n";
        LOG_DEBUG("Saved keybind " << action << " = " << (keyName ? keyName : "Unknown"));
    }
}