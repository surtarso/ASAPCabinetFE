#include "input/input_manager.h"
#include "utils/logging.h"

InputManager::InputManager(const IKeybindProvider& keybindProvider) : keybindProvider_(keybindProvider) {}

bool InputManager::isAction(const SDL_Event& event, const std::string& action) const {
    SDL_Keycode key = keybindProvider_.getKey(action);
    bool match = (event.type == SDL_KEYDOWN && event.key.keysym.sym == key);
    LOG_DEBUG("Checking action: " << action << ", key: " << SDL_GetKeyName(event.key.keysym.sym) 
              << " (keycode: " << event.key.keysym.sym << ") against " << SDL_GetKeyName(key) 
              << " (keycode: " << key << "), Match=" << match);
    return match;
}