#include "input/input_manager.h"
#include "config/config_loader.h"
#include "utils/logging.h"
#include <iostream>

bool InputManager::isKeyPressed(const SDL_Event& event, SDL_Keycode key) {
    if (event.type != SDL_KEYDOWN || key == SDLK_UNKNOWN) return false;
    bool match = event.key.keysym.sym == key;
    LOG_DEBUG("Checking key: Event=" << event.key.keysym.sym << ", Bound=" << key << ", Match=" << match);
    return match;
}

bool InputManager::isPreviousTable(const SDL_Event& event) {
    return isKeyPressed(event, KEY_PREVIOUS_TABLE);
}

bool InputManager::isNextTable(const SDL_Event& event) {
    return isKeyPressed(event, KEY_NEXT_TABLE);
}

bool InputManager::isFastPrevTable(const SDL_Event& event) {
    return isKeyPressed(event, KEY_FAST_PREV_TABLE);
}

bool InputManager::isFastNextTable(const SDL_Event& event) {
    return isKeyPressed(event, KEY_FAST_NEXT_TABLE);
}

bool InputManager::isJumpNextLetter(const SDL_Event& event) {
    return isKeyPressed(event, KEY_JUMP_NEXT_LETTER);
}

bool InputManager::isJumpPrevLetter(const SDL_Event& event) {
    return isKeyPressed(event, KEY_JUMP_PREV_LETTER);
}

bool InputManager::isLaunchTable(const SDL_Event& event) {
    return isKeyPressed(event, KEY_LAUNCH_TABLE);
}

bool InputManager::isToggleConfig(const SDL_Event& event) {
    return isKeyPressed(event, KEY_TOGGLE_CONFIG);
}

bool InputManager::isQuit(const SDL_Event& event) {
    return isKeyPressed(event, KEY_QUIT);
}

bool InputManager::isConfigSave(const SDL_Event& event) {
    return isKeyPressed(event, KEY_CONFIG_SAVE);
}

bool InputManager::isConfigClose(const SDL_Event& event) {
    return isKeyPressed(event, KEY_CONFIG_CLOSE);
}

bool InputManager::isScreenshotMode(const SDL_Event& event) {
    return isKeyPressed(event, KEY_SCREENSHOT_MODE);
}