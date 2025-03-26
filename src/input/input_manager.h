#ifndef INPUT_MANAGER_H
#define INPUT_MANAGER_H

#include <SDL2/SDL.h>
#include "config/settings.h"

class InputManager {
public:
    InputManager(const Settings& settings); // Take Settings in constructor

    // Key detection methods
    bool isKeyPressed(const SDL_Event& event, SDL_Keycode key) const;
    bool isPreviousTable(const SDL_Event& event) const;
    bool isNextTable(const SDL_Event& event) const;
    bool isFastPrevTable(const SDL_Event& event) const;
    bool isFastNextTable(const SDL_Event& event) const;
    bool isJumpNextLetter(const SDL_Event& event) const;
    bool isJumpPrevLetter(const SDL_Event& event) const;
    bool isLaunchTable(const SDL_Event& event) const;
    bool isToggleConfig(const SDL_Event& event) const;
    bool isQuit(const SDL_Event& event) const;
    bool isConfigSave(const SDL_Event& event) const;
    bool isConfigClose(const SDL_Event& event) const;
    bool isScreenshotMode(const SDL_Event& event) const;

private:
    const Settings& settings_; // Reference to settings
};

#endif // INPUT_MANAGER_H