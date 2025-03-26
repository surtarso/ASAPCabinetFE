#include "input_manager.h"
#include "utils/logging.h"

InputManager::InputManager(const Settings& settings) : settings_(settings) {}

bool InputManager::isKeyPressed(const SDL_Event& event, SDL_Keycode key) const {
    bool match = (event.type == SDL_KEYDOWN && event.key.keysym.sym == key);
    LOG_DEBUG("Checking key: " << SDL_GetKeyName(event.key.keysym.sym) << " against " << SDL_GetKeyName(key) << ", Match=" << match);
    return match;
}

bool InputManager::isPreviousTable(const SDL_Event& event) const {
    return isKeyPressed(event, settings_.keyPreviousTable);
}

bool InputManager::isNextTable(const SDL_Event& event) const {
    return isKeyPressed(event, settings_.keyNextTable);
}

bool InputManager::isFastPrevTable(const SDL_Event& event) const {
    return isKeyPressed(event, settings_.keyFastPrevTable);
}

bool InputManager::isFastNextTable(const SDL_Event& event) const {
    return isKeyPressed(event, settings_.keyFastNextTable);
}

bool InputManager::isJumpNextLetter(const SDL_Event& event) const {
    return isKeyPressed(event, settings_.keyJumpNextLetter);
}

bool InputManager::isJumpPrevLetter(const SDL_Event& event) const {
    return isKeyPressed(event, settings_.keyJumpPrevLetter);
}

bool InputManager::isLaunchTable(const SDL_Event& event) const {
    return isKeyPressed(event, settings_.keyLaunchTable);
}

bool InputManager::isToggleConfig(const SDL_Event& event) const {
    return isKeyPressed(event, settings_.keyToggleConfig);
}

bool InputManager::isQuit(const SDL_Event& event) const {
    return isKeyPressed(event, settings_.keyQuit);
}

bool InputManager::isConfigSave(const SDL_Event& event) const {
    return isKeyPressed(event, settings_.keyConfigSave);
}

bool InputManager::isConfigClose(const SDL_Event& event) const {
    return isKeyPressed(event, settings_.keyConfigClose);
}

bool InputManager::isScreenshotMode(const SDL_Event& event) const {
    return isKeyPressed(event, settings_.keyScreenshotMode);
}