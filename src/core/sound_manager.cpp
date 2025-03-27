#include "sound_manager.h"
#include "utils/logging.h"
#include <algorithm>
#include <iostream>

SoundManager::SoundManager(const std::string& exeDir, const Settings& settings)
    : exeDir_(exeDir), settings_(settings) {
    // Initialize all expected keys with nullptr and the deleter
    sounds_.emplace("table_change", std::unique_ptr<Mix_Chunk, void(*)(Mix_Chunk*)>(nullptr, Mix_FreeChunk));
    sounds_.emplace("table_load", std::unique_ptr<Mix_Chunk, void(*)(Mix_Chunk*)>(nullptr, Mix_FreeChunk));
    sounds_.emplace("config_toggle", std::unique_ptr<Mix_Chunk, void(*)(Mix_Chunk*)>(nullptr, Mix_FreeChunk));
}

SoundManager::~SoundManager() {
    sounds_.clear(); // unique_ptr handles cleanup via Mix_FreeChunk
    LOG_DEBUG("SoundManager destroyed");
}

void SoundManager::loadSounds() {
    auto loadSound = [&](const std::string& key, const std::string& path) {
        std::string fullPath = exeDir_ + trim(path);
        LOG_DEBUG("Loading sound '" << key << "' from: " << fullPath);
        // Use at() to ensure the key exists, avoiding operator[]'s default construction
        if (sounds_.find(key) != sounds_.end()) {
            sounds_.at(key).reset(Mix_LoadWAV(fullPath.c_str()));
            if (!sounds_.at(key)) {
                std::cerr << "Mix_LoadWAV Error for " << key << " at " << fullPath << ": " << Mix_GetError() << std::endl;
            } else {
                LOG_DEBUG("Sound '" << key << "' loaded successfully");
            }
        } else {
            LOG_DEBUG("Sound key '" << key << "' not found in map; skipping load");
        }
    };

    loadSound("table_change", settings_.tableChangeSound);
    loadSound("table_load", settings_.tableLoadSound);
    loadSound("config_toggle", settings_.configToggleSound);
}

void SoundManager::playSound(const std::string& key) {
    if (sounds_.count(key) && sounds_.at(key)) {
        LOG_DEBUG("Playing sound: " << key);
        Mix_PlayChannel(-1, sounds_.at(key).get(), 0);
    } else {
        LOG_DEBUG("Sound '" << key << "' not found or not loaded");
    }
}

std::string SoundManager::trim(const std::string& str) {
    std::string trimmed = str;
    trimmed.erase(std::remove_if(trimmed.begin(), trimmed.end(), isspace), trimmed.end());
    return trimmed;
}