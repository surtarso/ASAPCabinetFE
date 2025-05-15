#ifndef SDL_GUARDS_H
#define SDL_GUARDS_H

#include <iostream>
#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>
#include <SDL_mixer.h>
#include "utils/logging.h"

/**
 * @class SDLInitGuard
 * @brief A guard class to manage the initialization and cleanup of SDL.
 */
class SDLInitGuard {
public:
    bool success;
    SDLInitGuard(Uint32 flags) : success(false) {
        if (SDL_Init(flags) == 0) {
            success = true;
        } else {
            LOG_DEBUG("SDLGuards: SDL_Init Error: " << SDL_GetError());
        }
    }
    ~SDLInitGuard() {
        if (success) SDL_Quit();
    }
};

/**
 * @class IMGInitGuard
 * @brief A guard class to manage the initialization and cleanup of the SDL_image library.
 */
class IMGInitGuard {
public:
    int flags;
    IMGInitGuard(int flags) : flags(flags) {
        if (!(IMG_Init(flags) & flags)) {
            LOG_DEBUG("SDLGuards: IMG_Init Error: " << IMG_GetError());
            this->flags = 0;
        }
    }
    ~IMGInitGuard() {
        if (flags) IMG_Quit();
    }
};

/**
 * @class TTFInitGuard
 * @brief A guard class to manage the initialization and cleanup of the SDL_ttf library.
 */
class TTFInitGuard {
public:
    bool success;
    TTFInitGuard() : success(false) {
        if (TTF_Init() == 0) {
            success = true;
        } else {
            LOG_DEBUG("SDLGuards: TTF_Init Error: " << TTF_GetError());
        }
    }
    ~TTFInitGuard() {
        if (success) TTF_Quit();
    }
};

/**
 * @class MixerGuard
 * @brief A RAII class to manage SDL_mixer audio initialization and cleanup.
 */
class MixerGuard {
public:
    bool success;
    MixerGuard(int frequency, Uint16 format, int channels, int chunksize) : success(false) {
        if (Mix_OpenAudio(frequency, format, channels, chunksize) == 0) {
            success = true;
        } else {
            LOG_DEBUG("SDLGuards: SDL_mixer Error: " << Mix_GetError());
        }
    }
    ~MixerGuard() {
        if (success) Mix_CloseAudio();
    }
};

#endif // SDL_GUARDS_H