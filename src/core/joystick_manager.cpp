#include "core/joystick_manager.h"
#include "utils/logging.h"
#include <iostream>

JoystickManager::JoystickManager() {
    initializeJoysticks();
    LOG_INFO("JoystickManager: JoystickManager constructed");
}

JoystickManager::~JoystickManager() {
    cleanupJoysticks();
    LOG_INFO("JoystickManager: JoystickManager destroyed");
}

void JoystickManager::initializeJoysticks() {
    int numJoysticks = SDL_NumJoysticks();
    LOG_INFO("JoystickManager: Found " << numJoysticks << " joysticks");
    for (int i = 0; i < numJoysticks; ++i) {
        SDL_Joystick* joystick = SDL_JoystickOpen(i);
        if (joystick) {
            joysticks_.push_back(joystick);
            LOG_DEBUG("JoystickManager: Opened joystick " << i << ": " << SDL_JoystickName(joystick));
        } else {
            LOG_ERROR("JoystickManager: Failed to open joystick " << i << ": " << SDL_GetError());
        }
    }
}

void JoystickManager::cleanupJoysticks() {
    for (auto joystick : joysticks_) {
        if (joystick) SDL_JoystickClose(joystick);
    }
    joysticks_.clear();
}

void JoystickManager::addJoystick(int index) {
    SDL_Joystick* joystick = SDL_JoystickOpen(index);
    if (joystick) {
        joysticks_.push_back(joystick);
        LOG_DEBUG("JoystickManager: Added joystick: " << SDL_JoystickName(joystick));
    }
}

void JoystickManager::removeJoystick(SDL_JoystickID id) {
    for (auto it = joysticks_.begin(); it != joysticks_.end(); ++it) {
        if (SDL_JoystickInstanceID(*it) == id) {
            SDL_JoystickClose(*it);
            joysticks_.erase(it);
            LOG_DEBUG("JoystickManager: Removed joystick ID: " << id);
            break;
        }
    }
}