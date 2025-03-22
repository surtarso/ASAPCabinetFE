#ifndef INPUT_MANAGER_H
#define INPUT_MANAGER_H

#include <SDL.h>

class InputManager {
public:
    InputManager() {}
    bool isPreviousTable(const SDL_Event& event);
    bool isNextTable(const SDL_Event& event);
    bool isFastPrevTable(const SDL_Event& event);
    bool isFastNextTable(const SDL_Event& event);
    bool isJumpNextLetter(const SDL_Event& event);
    bool isJumpPrevLetter(const SDL_Event& event);
    bool isLaunchTable(const SDL_Event& event);
    bool isToggleConfig(const SDL_Event& event);
    bool isQuit(const SDL_Event& event);
    bool isConfigSave(const SDL_Event& event);
    bool isConfigClose(const SDL_Event& event);

private:
    bool isKeyPressed(const SDL_Event& event, SDL_Keycode key);
};

#endif // INPUT_MANAGER_H