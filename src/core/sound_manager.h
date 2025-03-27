#ifndef SOUND_MANAGER_H
#define SOUND_MANAGER_H

#include <SDL2/SDL_mixer.h>
#include <string>
#include <unordered_map>
#include <memory>
#include "config/settings.h"

class SoundManager {
public:
    SoundManager(const std::string& exeDir, const Settings& settings);
    ~SoundManager();

    // Load all sounds from settings
    void loadSounds();
    // Play a sound by its key
    void playSound(const std::string& key);

private:
    std::string exeDir_;
    const Settings& settings_; // Reference to settings for paths
    std::unordered_map<std::string, std::unique_ptr<Mix_Chunk, void(*)(Mix_Chunk*)>> sounds_;

    // Helper to trim whitespace
    std::string trim(const std::string& str);
};

#endif // SOUND_MANAGER_H