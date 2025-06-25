/**
 * @file joystick_manager.cpp
 * @brief Implementation of the JoystickManager class for handling SDL joysticks.
 */

#include "core/joystick_manager.h"
#include "log/logging.h"
#include <SDL2/SDL.h>
#include <string>

JoystickManager::JoystickManager() {
    initializeJoysticks();
    LOG_DEBUG("JoystickManager constructed");
}

JoystickManager::~JoystickManager() {
    cleanupJoysticks();
    LOG_DEBUG("JoystickManager destroyed");
}

void JoystickManager::initializeJoysticks() {
    int numJoysticks = SDL_NumJoysticks();
    LOG_INFO("Found " + std::to_string(numJoysticks) + " joysticks");
    for (int i = 0; i < numJoysticks; ++i) {
        SDL_Joystick* joystick = SDL_JoystickOpen(i);
        if (joystick) {
            joysticks_.push_back(joystick);
            LOG_DEBUG("Opened joystick " + std::to_string(i) + ": " + std::string(SDL_JoystickName(joystick)));
        } else {
            LOG_ERROR("Failed to open joystick " + std::to_string(i) + ": " + std::string(SDL_GetError()));
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
        LOG_DEBUG("Added joystick: " + std::string(SDL_JoystickName(joystick)));
    }
}

void JoystickManager::removeJoystick(SDL_JoystickID id) {
    for (auto it = joysticks_.begin(); it != joysticks_.end(); ++it) {
        if (SDL_JoystickInstanceID(*it) == id) {
            SDL_JoystickClose(*it);
            joysticks_.erase(it);
            LOG_DEBUG("Removed joystick ID: " + std::to_string(id));
            break;
        }
    }
}