#ifndef SOUND_MANAGER_H
#define SOUND_MANAGER_H

#include "sound/isound_manager.h"
#include <string>
#include <unordered_map>
#include <memory>
#include <SDL2/SDL_mixer.h>
#include "config/settings.h" // Make sure Settings is included

/**
 * @brief Manages all sound operations for the application.
 *
 * This class implements the ISoundManager interface, providing concrete
 * functionality for loading, playing, and controlling UI sounds (Mix_Chunk),
 * ambience music (Mix_Music), and table-specific music (Mix_Chunk on a dedicated channel).
 * It handles SDL_mixer initialization and resource management, allowing for
 * continuous ambience music playback alongside optional table music.
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
     * This includes UI sound effects. Ambience and table music are loaded dynamically.
     */
    void loadSounds() override;

    /**
     * @brief Plays a specific UI sound effect.
     * @param key The unique identifier for the UI sound.
     */
    void playUISound(const std::string& key) override;

    /**
     * @brief Plays the background ambience music.
     * If the same ambience music is already playing, its volume settings are updated.
     * Otherwise, the new ambience music is loaded and played, persistently looping
     * across table changes unless explicitly stopped or muted.
     * @param path The full path to the ambience music file.
     */
    void playAmbienceMusic(const std::string& path) override;

    /**
     * @brief Plays the table-specific music on a dedicated channel.
     * This music plays concurrently with the ambience music. If no valid
     * table music path is provided or the file is not found, any currently
     * playing table music is stopped, and ambience music (if configured)
     * is ensured to be playing.
     * @param path The full path to the table music file.
     */
    void playTableMusic(const std::string& path) override;

    /**
     * @brief Stops all currently playing music (ambience and table music).
     * This method will halt both Mix_Music and the dedicated Mix_Chunk channel.
     * For more granular control, use stopAmbienceMusic() or stopTableMusic().
     */
    void stopMusic() override;

    /**
     * @brief Applies the current audio settings (volumes, mute states) to all active sound types.
     */
    void applyAudioSettings() override;

    /**
     * @brief Updates the internal settings and reloads sounds if necessary.
     * UI sounds are reloaded if their paths change. Ambience music is managed
     * to ensure continuous playback unless its path becomes invalid or it's muted.
     * Table music volume/mute settings are reapplied.
     * @param newSettings The new settings to apply.
     */
    void updateSettings(const Settings& newSettings) override;

private:
    std::string exeDir_; ///< The executable directory.
    Settings settings_;  ///< A local copy of the current application settings.

    // Removed: MusicType currentPlayingMusicType_;

    // Separate storage for UI sounds (Mix_Chunk are effects, can play concurrently)
    std::unordered_map<std::string, std::unique_ptr<Mix_Chunk, void(*)(Mix_Chunk*)>> uiSounds_; ///< Map of UI sound effects.
    
    // Ambience music (Mix_Music for background looping music)
    std::unique_ptr<Mix_Music, void(*)(Mix_Music*)> ambienceMusic_; ///< Pointer to the loaded ambience music.
    std::string lastAmbiencePath_; ///< Stores the path of the currently loaded ambience music to prevent unnecessary reloads.

    // Table music (NOW Mix_Chunk to allow concurrent playback with Mix_Music)
    // IMPORTANT: Table music files should be WAV/OGG and reasonable size, as Mix_Chunk loads entirely into RAM.
    std::unique_ptr<Mix_Chunk, void(*)(Mix_Chunk*)> tableMusic_;    ///< Pointer to the loaded table music (now a Mix_Chunk).
    std::string lastTableMusicPath_; ///< Stores the path of the currently loaded table music to prevent unnecessary reloads.
    int tableMusicChannel_;          ///< The dedicated SDL_mixer channel used for table music.

    // Track playing state for ambience and table music independently
    bool isAmbiencePlaying_ = false; ///< True if ambience music is actively playing.
    bool isTableMusicPlaying_ = false; ///< True if table music is actively playing.


    /**
     * @brief Helper to load UI sounds (Mix_Chunk).
     * This function is separated for clarity and reusability.
     * @param key The unique identifier for the UI sound.
     * @param path The relative path to the sound file.
     */
    void loadUiSound(const std::string& key, const std::string& path);

    /**
     * @brief Stops only the background ambience music.
     * This halts the Mix_Music channel.
     */
    void stopAmbienceMusic();

    /**
     * @brief Stops only the table-specific music.
     * This halts playback on the dedicated table music channel.
     */
    void stopTableMusic();

    /**
     * @brief Trims leading and trailing whitespace from a string.
     * @param str The string to trim.
     * @return The trimmed string.
     */
    std::string trim(const std::string& str);
};

#endif // SOUND_MANAGER_H