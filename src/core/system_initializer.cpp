#include "core/system_initializer.h"
#include "utils/logging.h"
#include <iostream>

SystemInitializer::SystemInitializer()
    : sdlGuard_(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_JOYSTICK),
      mixerGuard_(44100, MIX_DEFAULT_FORMAT, 2, 2048),
      ttfGuard_(),
      imgGuard_(IMG_INIT_PNG | IMG_INIT_JPG) {
    if (!isInitialized()) {
        std::cerr << "System initialization failed" << std::endl;
        exit(1);
    }
    if (Mix_Init(MIX_INIT_MP3) != MIX_INIT_MP3) {
        std::cerr << "Mix_Init Error: " << Mix_GetError() << std::endl;
        exit(1);
    }
    initializeJoysticks();
    LOG_DEBUG("SystemInitializer constructed");
}

SystemInitializer::~SystemInitializer() {
    cleanupJoysticks();
    LOG_DEBUG("SystemInitializer destroyed");
}

void SystemInitializer::initializeJoysticks() {
    int numJoysticks = SDL_NumJoysticks();
    LOG_DEBUG("Found " << numJoysticks << " joysticks");
    for (int i = 0; i < numJoysticks; ++i) {
        SDL_Joystick* joystick = SDL_JoystickOpen(i);
        if (joystick) {
            joysticks_.push_back(joystick);
            LOG_DEBUG("Opened joystick " << i << ": " << SDL_JoystickName(joystick));
        } else {
            LOG_DEBUG("Failed to open joystick " << i << ": " << SDL_GetError());
        }
    }
}

void SystemInitializer::cleanupJoysticks() {
    for (auto joystick : joysticks_) {
        if (joystick) SDL_JoystickClose(joystick);
    }
    joysticks_.clear();
}

void SystemInitializer::addJoystick(int index) {
    SDL_Joystick* joystick = SDL_JoystickOpen(index);
    if (joystick) {
        joysticks_.push_back(joystick);
        LOG_DEBUG("Added joystick: " << SDL_JoystickName(joystick));
    }
}

void SystemInitializer::removeJoystick(SDL_JoystickID id) {
    for (auto it = joysticks_.begin(); it != joysticks_.end(); ++it) {
        if (SDL_JoystickInstanceID(*it) == id) {
            SDL_JoystickClose(*it);
            joysticks_.erase(it);
            LOG_DEBUG("Removed joystick ID: " << id);
            break;
        }
    }
}