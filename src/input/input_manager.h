#ifndef INPUT_MANAGER_H
#define INPUT_MANAGER_H

#include <SDL2/SDL.h>
#include "keybinds/ikeybind_provider.h"

class InputManager {
public:
    InputManager(const IKeybindProvider& keybindProvider);
    bool isAction(const SDL_Event& event, const std::string& action) const;

private:
    const IKeybindProvider& keybindProvider_;
};

#endif // INPUT_MANAGER_H