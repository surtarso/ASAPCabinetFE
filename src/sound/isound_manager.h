#ifndef ISOUND_MANAGER_H
#define ISOUND_MANAGER_H

#include <string>
#include "config/settings.h" // Include Settings for updateSettings

/**
 * @brief Interface for managing sound operations.
 *
 * The ISoundManager class provides an abstract interface for loading and playing sounds within the application.
 * It distinguishes between UI sound effects (Mix_Chunk), background ambience music (Mix_Music),
 * and table-specific music (Mix_Chunk playing on a dedicated channel), providing methods for each.
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
     * when sound-related settings change. It primarily loads UI sounds; ambience
     * and table music are loaded dynamically as needed.
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
     * This music typically loops and plays continuously, designed to persist
     * across table selections. If the same ambience music is already playing,
     * its volume settings are updated. If the path is empty or invalid,
     * any currently playing ambience music is stopped.
     * @param path The full path to the ambience music file (e.g., MP3, OGG).
     */
    virtual void playAmbienceMusic(const std::string& path) = 0;

    /**
     * @brief Plays the table-specific music on a dedicated channel.
     * This music plays concurrently with the ambience music. It is recommended
     * that table music files are in a format suitable for Mix_Chunk (e.g., WAV, OGG)
     * and of reasonable size, as they are loaded entirely into memory.
     * If the path is empty or invalid, any currently playing table music is stopped.
     * @param path The full path to the table music file.
     */
    virtual void playTableMusic(const std::string& path) = 0;

    /**
     * @brief Stops all currently playing background music.
     * This includes both the ambience music and the table-specific music.
     */
    virtual void stopMusic() = 0;

    /**
     * @brief Applies the current audio settings (volumes, mute states) to all active sound types.
     * This should be called after settings are updated or on initialization.
     */
    virtual void applyAudioSettings() = 0;

    /**
     * @brief Updates the internal settings and reloads sounds if necessary.
     * UI sounds are reloaded if their paths change. Ambience music is managed
     * to ensure continuous playback unless its path becomes invalid or it's muted.
     * Table music volume/mute settings are reapplied.
     * @param newSettings The new settings to apply.
     */
    virtual void updateSettings(const Settings& newSettings) = 0;
};

#endif // ISOUND_MANAGER_H