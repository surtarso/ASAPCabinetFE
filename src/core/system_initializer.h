#ifndef SYSTEM_INITIALIZER_H
#define SYSTEM_INITIALIZER_H

#include "utils/sdl_guards.h"
#include <SDL2/SDL.h>
#include <vector>

class SystemInitializer {
public:
    SystemInitializer();
    ~SystemInitializer();
    std::vector<SDL_Joystick*> getJoysticks() const { return joysticks_; }
    bool isInitialized() const { return sdlGuard_.success && mixerGuard_.success && ttfGuard_.success && imgGuard_.flags; }
    void addJoystick(int index);
    void removeJoystick(SDL_JoystickID id);

private:
    SDLInitGuard sdlGuard_;
    MixerGuard mixerGuard_;
    TTFInitGuard ttfGuard_;
    IMGInitGuard imgGuard_;
    std::vector<SDL_Joystick*> joysticks_;

    void initializeJoysticks();
    void cleanupJoysticks();
};

#endif // SYSTEM_INITIALIZER_H