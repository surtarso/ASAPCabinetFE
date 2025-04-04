#ifndef SOUND_MANAGER_H
#define SOUND_MANAGER_H

#include "sound/isound_manager.h"
#include <string>
#include <unordered_map>
#include <memory>
#include <SDL2/SDL_mixer.h>
#include "config/settings.h"

class SoundManager : public ISoundManager {
public:
    SoundManager(const std::string& exeDir, const Settings& settings);
    ~SoundManager() override;
    void loadSounds() override;
    void playSound(const std::string& key) override;

private:
    std::string exeDir_;
    Settings settings_;  // Note: Stored by value, not ref—hope that’s intentional
    std::unordered_map<std::string, std::unique_ptr<Mix_Music, void(*)(Mix_Music*)>> sounds_;
    std::string trim(const std::string& str);
};

#endif // SOUND_MANAGER_H