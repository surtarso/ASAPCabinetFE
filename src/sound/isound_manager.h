#ifndef ISOUND_MANAGER_H
#define ISOUND_MANAGER_H

#include <string>
#include "config/settings.h" // Include Settings for updateSettings

/**
 * @brief Interface for managing sound operations.
 *
 * The ISoundManager class provides an abstract interface for loading and playing sounds within the application.
 * It distinguishes between UI sound effects (Mix_Chunk), background ambience music (Mix_Music),
 * and table-specific music (Mix_Music), providing methods for each.
 */
class ISoundManager {
public:
    /**
     * @brief Virtual destructor to ensure proper cleanup for derived classes.
     */
    virtual ~ISoundManager() = default;

    /**
     * @brief Loads all necessary sound resources for the application.
     *
     * This method is typically called once during application initialization and
     * when sound-related settings change.
     */
    virtual void loadSounds() = 0;

    /**
     * @brief Plays a specific UI sound effect identified by its unique key.
     * These sounds are typically short and can play concurrently with music.
     * @param key The unique string identifier for the UI sound to be played.
     */
    virtual void playUISound(const std::string& key) = 0;

    /**
     * @brief Plays the background ambience music.
     * This music typically loops and plays continuously unless overridden by table music.
     * @param path The full path to the ambience music file. If empty, stops current ambience.
     */
    virtual void playAmbienceMusic(const std::string& path) = 0;

    /**
     * @brief Plays the table-specific music.
     * This music typically loops and will stop any currently playing ambience music.
     * If the path is empty or invalid, it will attempt to resume ambience music.
     * @param path The full path to the table music file. If empty, stops current table music.
     */
    virtual void playTableMusic(const std::string& path) = 0;

    /**
     * @brief Stops any currently playing background music (ambience or table music).
     */
    virtual void stopMusic() = 0;

    /**
     * @brief Applies the current audio settings (volumes, mute states) to all active sound types.
     * This should be called after settings are updated or on initialization.
     */
    virtual void applyAudioSettings() = 0;

    /**
     * @brief Updates the internal settings and reloads sounds if necessary.
     * @param newSettings The new settings to apply.
     */
    virtual void updateSettings(const Settings& newSettings) = 0;
};

#endif // ISOUND_MANAGER_H
