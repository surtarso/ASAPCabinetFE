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

    sounds_.emplace("scroll_prev", std::unique_ptr<Mix_Chunk, void(*)(Mix_Chunk*)>(nullptr, Mix_FreeChunk));
    sounds_.emplace("scroll_next", std::unique_ptr<Mix_Chunk, void(*)(Mix_Chunk*)>(nullptr, Mix_FreeChunk));
    sounds_.emplace("scroll_fast_prev", std::unique_ptr<Mix_Chunk, void(*)(Mix_Chunk*)>(nullptr, Mix_FreeChunk));
    sounds_.emplace("scroll_fast_next", std::unique_ptr<Mix_Chunk, void(*)(Mix_Chunk*)>(nullptr, Mix_FreeChunk));
    sounds_.emplace("scroll_jump_prev", std::unique_ptr<Mix_Chunk, void(*)(Mix_Chunk*)>(nullptr, Mix_FreeChunk));
    sounds_.emplace("scroll_jump_next", std::unique_ptr<Mix_Chunk, void(*)(Mix_Chunk*)>(nullptr, Mix_FreeChunk));
    sounds_.emplace("scroll_random", std::unique_ptr<Mix_Chunk, void(*)(Mix_Chunk*)>(nullptr, Mix_FreeChunk));
    sounds_.emplace("launch_table", std::unique_ptr<Mix_Chunk, void(*)(Mix_Chunk*)>(nullptr, Mix_FreeChunk));
    sounds_.emplace("launch_screenshot", std::unique_ptr<Mix_Chunk, void(*)(Mix_Chunk*)>(nullptr, Mix_FreeChunk));
    sounds_.emplace("config_save", std::unique_ptr<Mix_Chunk, void(*)(Mix_Chunk*)>(nullptr, Mix_FreeChunk));
    sounds_.emplace("config_close", std::unique_ptr<Mix_Chunk, void(*)(Mix_Chunk*)>(nullptr, Mix_FreeChunk));
    sounds_.emplace("quit", std::unique_ptr<Mix_Chunk, void(*)(Mix_Chunk*)>(nullptr, Mix_FreeChunk));
    sounds_.emplace("screenshot_take", std::unique_ptr<Mix_Chunk, void(*)(Mix_Chunk*)>(nullptr, Mix_FreeChunk));
    sounds_.emplace("screenshot_quit", std::unique_ptr<Mix_Chunk, void(*)(Mix_Chunk*)>(nullptr, Mix_FreeChunk));
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

    loadSound("config_toggle", settings_.configToggleSound);
    loadSound("scroll_prev", settings_.scrollPrevSound);
    loadSound("scroll_next", settings_.scrollNextSound);
    loadSound("scroll_fast_prev", settings_.scrollFastPrevSound);
    loadSound("scroll_fast_next", settings_.scrollFastNextSound);
    loadSound("scroll_jump_prev", settings_.scrollJumpPrevSound);
    loadSound("scroll_jump_next", settings_.scrollJumpNextSound);
    loadSound("scroll_random", settings_.scrollRandomSound);
    loadSound("launch_table", settings_.launchTableSound);
    loadSound("launch_screenshot", settings_.launchScreenshotSound);
    loadSound("config_save", settings_.configSaveSound);
    loadSound("config_close", settings_.configCloseSound);
    loadSound("quit", settings_.quitSound);
    loadSound("screenshot_take", settings_.screenshotTakeSound);
    loadSound("screenshot_quit", settings_.screenshotQuitSound);
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