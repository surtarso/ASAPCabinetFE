/**
 * @file sdl_guards.h
 * @brief Defines RAII guard classes for SDL and its extensions in ASAPCabinetFE.
 *
 * This header provides guard classes (SDLInitGuard, IMGInitGuard, TTFInitGuard,
 * MixerGuard) to manage the initialization and cleanup of SDL and its extension
 * libraries (SDL_image, SDL_ttf, SDL_mixer) using RAII principles, ensuring resources
 * are properly released.
 */

#ifndef SDL_GUARDS_H
#define SDL_GUARDS_H

#include <iostream>
#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>
#include <SDL_mixer.h>
#include "log/logging.h"

/**
 * @class SDLInitGuard
 * @brief RAII guard for SDL initialization and cleanup.
 *
 * This class initializes SDL with the specified flags and ensures SDL_Quit is called
 * upon destruction if initialization succeeds. It logs errors using LOG_DEBUG if
 * initialization fails.
 */
class SDLInitGuard {
public:
    /**
     * @brief Constructs an SDLInitGuard and initializes SDL.
     *
     * Attempts to initialize SDL with the provided flags, setting success to true if
     * successful, or logging an error if it fails.
     *
     * @param flags The SDL initialization flags (e.g., SDL_INIT_VIDEO).
     */
    SDLInitGuard(Uint32 flags);

    /**
     * @brief Destroys the SDLInitGuard and cleans up SDL.
     *
     * Calls SDL_Quit if initialization was successful.
     */
    ~SDLInitGuard();

    bool success; ///< True if SDL initialization succeeded, false otherwise.
};

/**
 * @class IMGInitGuard
 * @brief RAII guard for SDL_image initialization and cleanup.
 *
 * This class initializes SDL_image with the specified flags and ensures IMG_Quit is
 * called upon destruction if initialization succeeds. It logs errors using LOG_DEBUG
 * if initialization fails.
 */
class IMGInitGuard {
public:
    /**
     * @brief Constructs an IMGInitGuard and initializes SDL_image.
     *
     * Attempts to initialize SDL_image with the provided flags, resetting flags to 0
     * and logging an error if initialization fails.
     *
     * @param flags The SDL_image initialization flags (e.g., IMG_INIT_PNG).
     */
    IMGInitGuard(int flags);

    /**
     * @brief Destroys the IMGInitGuard and cleans up SDL_image.
     *
     * Calls IMG_Quit if initialization was successful (flags non-zero).
     */
    ~IMGInitGuard();

    int flags; ///< The SDL_image initialization flags, 0 if initialization failed.
};

/**
 * @class TTFInitGuard
 * @brief RAII guard for SDL_ttf initialization and cleanup.
 *
 * This class initializes SDL_ttf and ensures TTF_Quit is called upon destruction if
 * initialization succeeds. It logs errors using LOG_DEBUG if initialization fails.
 */
class TTFInitGuard {
public:
    /**
     * @brief Constructs a TTFInitGuard and initializes SDL_ttf.
     *
     * Attempts to initialize SDL_ttf, setting success to true if successful, or
     * logging an error if it fails.
     */
    TTFInitGuard();

    /**
     * @brief Destroys the TTFInitGuard and cleans up SDL_ttf.
     *
     * Calls TTF_Quit if initialization was successful.
     */
    ~TTFInitGuard();

    bool success; ///< True if SDL_ttf initialization succeeded, false otherwise.
};

/**
 * @class MixerGuard
 * @brief RAII guard for SDL_mixer audio initialization and cleanup.
 *
 * This class initializes SDL_mixer audio with the specified parameters and ensures
 * Mix_CloseAudio is called upon destruction if initialization succeeds. It logs errors
 * using LOG_DEBUG if initialization fails.
 */
class MixerGuard {
public:
    /**
     * @brief Constructs a MixerGuard and initializes SDL_mixer audio.
     *
     * Attempts to open an audio device with the specified parameters, setting success
     * to true if successful, or logging an error if it fails.
     *
     * @param frequency The audio frequency (e.g., 44100 Hz).
     * @param format The audio format (e.g., AUDIO_S16SYS).
     * @param channels The number of audio channels (e.g., 2 for stereo).
     * @param chunksize The audio buffer size (e.g., 2048).
     */
    MixerGuard(int frequency, Uint16 format, int channels, int chunksize);

    /**
     * @brief Destroys the MixerGuard and cleans up SDL_mixer audio.
     *
     * Calls Mix_CloseAudio if initialization was successful.
     */
    ~MixerGuard();

    bool success; ///< True if SDL_mixer audio initialization succeeded, false otherwise.
};

#endif // SDL_GUARDS_H