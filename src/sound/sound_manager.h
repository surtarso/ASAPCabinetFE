#ifndef SOUND_MANAGER_H
#define SOUND_MANAGER_H

#include "sound/isound_manager.h"
#include <string>
#include <unordered_map>
#include <memory>
#include <SDL2/SDL_mixer.h>
#include "config/settings.h" // Make sure Settings is included
#include <random>

/**
 * @brief Manages all sound operations for the application.
 *
 * This class implements the ISoundManager interface, providing concrete
 * functionality for loading, playing, and controlling UI sounds (Mix_Chunk),
 * ambience music (Mix_Music), and table-specific music (Mix_Music).
 * It handles SDL_mixer initialization and resource management.
 */
class SoundManager : public ISoundManager {
public:
    /**
     * @brief Constructs a SoundManager instance.
     * @param exeDir The executable directory, used for resolving relative sound paths.
     * @param settings Initial application settings, including sound paths and volumes.
     */
    SoundManager(const std::string& exeDir, const Settings& settings);

    /**
     * @brief Destroys the SoundManager instance and cleans up all SDL_mixer resources.
     */
    ~SoundManager() override;

    /**
     * @brief Loads all necessary sound resources based on current settings.
     * This includes UI sound effects and the ambience music file.
     */
    void loadSounds() override;

    /**
     * @brief Plays a specific UI sound effect.
     * @param key The unique identifier for the UI sound.
     */
    void playUISound(const std::string& key) override;

    /**
     * @brief Plays the background ambience music.
     * @param path The full path to the ambience music file.
     */
    void playAmbienceMusic(const std::string& path) override;

    /**
     * @brief Plays the table-specific music.
     * @param path The full path to the table music file.
     */
    void playTableMusic(const std::string& path) override;

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
     * @param newSettings The new settings to apply.
     */
    void updateSettings(const Settings& newSettings) override;

private:
    std::string exeDir_; ///< The executable directory.
    Settings settings_;  ///< A local copy of the current application settings.

    // Enum to track which type of music is currently intended to be playing by our manager
    enum class MusicType {
        None,
        Ambience,
        Table
    };
    MusicType currentPlayingMusicType_; ///< Tracks the type of music currently playing (or intended to play).

    // Separate storage for UI sounds (Mix_Chunk) and music (Mix_Music)
    std::unordered_map<std::string, std::unique_ptr<Mix_Chunk, void(*)(Mix_Chunk*)>> uiSounds_; ///< Map of UI sound effects.
    std::unique_ptr<Mix_Music, void(*)(Mix_Music*)> ambienceMusic_; ///< Pointer to the loaded ambience music.
    std::unique_ptr<Mix_Music, void(*)(Mix_Music*)> tableMusic_;    ///< Pointer to the loaded table music.
    std::mt19937 rng_; // Mersenne Twister engine
    std::uniform_real_distribution<double> dist_; // Distribution for random double

    /**
     * @brief Trims leading and trailing whitespace from a string.
     * @param str The string to trim.
     * @return The trimmed string.
     */
    std::string trim(const std::string& str);
};

#endif // SOUND_MANAGER_H
