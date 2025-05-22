#ifndef ISOUND_MANAGER_H
#define ISOUND_MANAGER_H

#include <string>

/**
 * @brief Interface for managing sound operations.
 *
 * The ISoundManager class provides an abstract interface for loading and playing sounds within the application.
 * Implementations of this interface should provide mechanisms to load sound resources
 * (e.g., from a configuration file or a specific directory)
 * and play them back using a string key identifier.
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
     * Implementations should define how and from where sounds are loaded
     * (e.g., parsing a configuration file, scanning a sound directory).
     * This method is typically called once during application initialization.
     */
    virtual void loadSounds() = 0;

    /**
     * @brief Plays a specific sound identified by its unique key.
     * @param key The unique string identifier for the sound to be played.
     * This key should correspond to a sound resource loaded via `loadSounds()`.
     */
    virtual void playSound(const std::string& key) = 0;
};

#endif // ISOUND_MANAGER_H
