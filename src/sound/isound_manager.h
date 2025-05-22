#ifndef ISOUND_MANAGER_H
#define ISOUND_MANAGER_H

#include <string>

/**
 * @brief Interface for managing sound operations.
 *
 * The ISoundManager class provides an abstract interface for loading and playing sounds.
 * Implementations of this interface should provide mechanisms to load sound resources
 * and play them using a string key identifier.
 */
class ISoundManager {
public:
    virtual ~ISoundManager() = default;
    virtual void loadSounds() = 0;
    virtual void playSound(const std::string& key) = 0;
};

#endif // ISOUND_MANAGER_H