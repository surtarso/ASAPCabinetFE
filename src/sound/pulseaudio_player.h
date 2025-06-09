/**
 * @file sound_manager.h
 * @brief Defines the PulseAudioPlayer class for managing sound operations in ASAPCabinetFE.
 *
 * This header provides the PulseAudioPlayer class, which implements the ISoundManager
 * interface to handle loading, playing, and controlling UI sounds (Mix_Chunk), ambience
 * music (Mix_Music), and table-specific music (Mix_Music) using SDL_mixer.
 */

#ifndef PULSEAUDIO_PLAYER_H
#define PULSEAUDIO_PLAYER_H

#include "sound/isound_manager.h"
#include <string>
#include <unordered_map>
#include <memory>
#include <SDL2/SDL_mixer.h>
#include <random>

/**
 * @class PulseAudioPlayer
 * @brief Manages all sound operations for the application.
 *
 * This class implements the ISoundManager interface, providing concrete functionality
 * for loading, playing, and controlling UI sounds (Mix_Chunk), ambience music (Mix_Music),
 * and table-specific music (Mix_Music). It handles SDL_mixer initialization and resource
 * management, including random positioning for ambience music.
 */
class PulseAudioPlayer : public ISoundManager {
public:
    /**
     * @brief Constructs a PulseAudioPlayer instance.
     *
     * Initializes the sound manager with the executable directory and initial settings.
     *
     * @param exeDir The executable directory, used for resolving relative sound paths.
     * @param settings Initial application settings, including sound paths and volumes.
     */
    PulseAudioPlayer(const std::string& exeDir, const Settings& settings);

    /**
     * @brief Destroys the PulseAudioPlayer instance and cleans up all SDL_mixer resources.
     *
     * Ensures proper cleanup of audio resources and SDL_mixer subsystems.
     */
    ~PulseAudioPlayer() override;

    /**
     * @brief Loads all necessary sound resources based on current settings.
     *
     * This includes UI sound effects and the ambience music file.
     */
    void loadSounds() override;

    /**
     * @brief Plays a specific UI sound effect.
     *
     * @param key The unique identifier for the UI sound.
     */
    void playUISound(const std::string& key) override;

    /**
     * @brief Plays the background ambience music.
     *
     * @param path The full path to the ambience music file.
     */
    void playAmbienceMusic(const std::string& path) override;

    /**
     * @brief Plays the table-specific music.
     *
     * @param path The full path to the table music file.
     */
    void playTableMusic(const std::string& path) override;

    /**
     * @brief Plays the custom launch audio.
     *
     * @param path The full path to the custom launch audio file.
     */
    void playCustomLaunch(const std::string& path) override;

    /**
     * @brief Stops any currently playing background music (ambience or table music).
     */
    void stopMusic() override;

    /**
     * @brief Applies the current audio settings (volumes, mute states) to all active sound types.
     */
    void applyAudioSettings() override;

    /**
     * @brief Updates the internal settings and reloads sounds if necessary.
     *
     * @param newSettings The new settings to apply.
     */
    void updateSettings(const Settings& newSettings) override;

private:
    std::string exeDir_; ///< The executable directory for resolving sound file paths.
    Settings settings_;  ///< A local copy of the current application settings.

    // Enum to track which type of music is currently intended to be playing by our manager
    enum class MusicType {
        None,
        Ambience,
        Table,
        Launch
    };
    MusicType currentPlayingMusicType_; ///< Tracks the type of music currently playing (or intended to play).

    // Separate storage for UI sounds (Mix_Chunk) and music (Mix_Music)
    std::unordered_map<std::string, std::unique_ptr<Mix_Chunk, void(*)(Mix_Chunk*)>> uiSounds_; ///< Map of UI sound effects.
    std::unique_ptr<Mix_Music, void(*)(Mix_Music*)> ambienceMusic_; ///< Pointer to the loaded ambience music.
    std::unique_ptr<Mix_Music, void(*)(Mix_Music*)> tableMusic_;    ///< Pointer to the loaded table music.
    std::unique_ptr<Mix_Music, void(*)(Mix_Music*)> launchAudio_;   ///< Pointer to the loaded launch audio (custom).
    std::mt19937 rng_;                                            ///< Mersenne Twister random number generator.
    std::uniform_real_distribution<double> dist_;                 ///< Distribution for random music positioning.

    /**
     * @brief Trims leading and trailing whitespace from a string.
     *
     * @param str The string to trim.
     * @return The trimmed string.
     */
    std::string trim(const std::string& str);
};

#endif // PULSEAUDIO_PLAYER_H