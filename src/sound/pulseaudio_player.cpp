/**
 * @file pulseaudio_player.cpp
 * @brief Implementation of the PulseAudioPlayer class for managing audio playback with SDL_mixer.
 */

#include "sound/pulseaudio_player.h"
#include "log/logging.h"
#include <filesystem>
#include <thread>
#include <chrono>
#include <string>

// Flag to track audio initialization
static bool audio_initialized = false;

PulseAudioPlayer::PulseAudioPlayer(const Settings& settings)
    : settings_(settings),
      currentPlayingMusicType_(MusicType::None),
      ambienceMusic_(nullptr, Mix_FreeMusic),
      tableMusic_(nullptr, Mix_FreeMusic),
      launchAudio_(nullptr, Mix_FreeMusic),
      rng_(std::chrono::steady_clock::now().time_since_epoch().count()),
      dist_(0.0, 1.0) {
    if (!audio_initialized) {
        if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
            LOG_ERROR("Mix_OpenAudio failed: " + std::string(Mix_GetError()));
            throw std::runtime_error("Failed to initialize audio");
        }
        int flags = MIX_INIT_MP3 | MIX_INIT_OGG;
        if (Mix_Init(flags) != flags) {
            LOG_ERROR("Mix_Init failed: " + std::string(Mix_GetError()));
            Mix_CloseAudio();
            throw std::runtime_error("Failed to initialize MP3/OGG support");
        }
        audio_initialized = true;
        LOG_DEBUG("SDL_mixer initialized with MP3 and OGG support");
    } else {
        LOG_DEBUG("SDL_mixer already initialized");
    }
    // Initialize UI sounds map with unique entries
    uiSounds_.emplace("panel_toggle", std::unique_ptr<Mix_Chunk, void(*)(Mix_Chunk*)>(nullptr, Mix_FreeChunk));
    uiSounds_.emplace("scroll_normal", std::unique_ptr<Mix_Chunk, void(*)(Mix_Chunk*)>(nullptr, Mix_FreeChunk));
    uiSounds_.emplace("scroll_fast", std::unique_ptr<Mix_Chunk, void(*)(Mix_Chunk*)>(nullptr, Mix_FreeChunk));
    uiSounds_.emplace("scroll_jump", std::unique_ptr<Mix_Chunk, void(*)(Mix_Chunk*)>(nullptr, Mix_FreeChunk));
    uiSounds_.emplace("scroll_random", std::unique_ptr<Mix_Chunk, void(*)(Mix_Chunk*)>(nullptr, Mix_FreeChunk));
    uiSounds_.emplace("launch_table", std::unique_ptr<Mix_Chunk, void(*)(Mix_Chunk*)>(nullptr, Mix_FreeChunk));
    uiSounds_.emplace("launch_screenshot", std::unique_ptr<Mix_Chunk, void(*)(Mix_Chunk*)>(nullptr, Mix_FreeChunk));
    uiSounds_.emplace("screenshot_take", std::unique_ptr<Mix_Chunk, void(*)(Mix_Chunk*)>(nullptr, Mix_FreeChunk));
    // Cache paths for music
    cachedAmbiencePath_ = "";
    cachedTableMusicPath_ = "";
    cachedLaunchAudioPath_ = "";
}

PulseAudioPlayer::~PulseAudioPlayer() {
    stopMusic();
    Mix_HaltChannel(-1);
    uiSounds_.clear();
    ambienceMusic_.reset();
    tableMusic_.reset();
    launchAudio_.reset();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    if (audio_initialized) {
        Mix_CloseAudio();
        Mix_Quit();
        audio_initialized = false;
        LOG_DEBUG("PulseAudioPlayer destroyed and SDL_mixer quit");
    }
}

void PulseAudioPlayer::loadSounds() {
    LOG_DEBUG("Loading sounds...");
    auto loadUiSound = [&](const std::string& key, const std::string& path) {
        if (path.empty()) {
            LOG_DEBUG("UI sound path is empty for key: " + key);
            uiSounds_.at(key).reset();
            return;
        }
        if (std::filesystem::exists(path) && std::filesystem::is_regular_file(path)) {
            uiSounds_.at(key).reset(Mix_LoadWAV(path.c_str()));
            if (!uiSounds_.at(key)) {
                LOG_ERROR("Mix_LoadWAV Error for " + key + " at " + path + ": " + std::string(Mix_GetError()));
            }
        } else {
            LOG_ERROR("UI sound file not found or not a regular file for " + key + " at " + path);
            uiSounds_.at(key).reset();
        }
    };
    loadUiSound("panel_toggle", settings_.panelToggleSound);
    loadUiSound("scroll_normal", settings_.scrollNormalSound);
    loadUiSound("scroll_fast", settings_.scrollFastSound);
    loadUiSound("scroll_jump", settings_.scrollJumpSound);
    loadUiSound("scroll_random", settings_.scrollRandomSound);
    loadUiSound("launch_table", settings_.launchTableSound);
    loadUiSound("launch_screenshot", settings_.launchScreenshotSound);
    loadUiSound("screenshot_take", settings_.screenshotTakeSound);
    if (!settings_.ambienceSound.empty() && std::filesystem::exists(settings_.ambienceSound) && std::filesystem::is_regular_file(settings_.ambienceSound)) {
        if (cachedAmbiencePath_ != settings_.ambienceSound) {
            ambienceMusic_.reset(Mix_LoadMUS(settings_.ambienceSound.c_str()));
            if (!ambienceMusic_) {
                LOG_ERROR("Mix_LoadMUS Error for ambience at " + settings_.ambienceSound + ": " + std::string(Mix_GetError()));
            } else {
                cachedAmbiencePath_ = settings_.ambienceSound;
                LOG_DEBUG("Ambience sound loaded and cached from " + settings_.ambienceSound);
            }
        } else {
            LOG_DEBUG("Ambience sound already cached: " + settings_.ambienceSound);
        }
    } else {
        LOG_INFO("Ambience sound path is empty or invalid. Ambience will not play.");
        ambienceMusic_.reset();
        cachedAmbiencePath_ = "";
    }
    LOG_INFO("PulseAudio Player Initialized.");
}

void PulseAudioPlayer::playUISound(const std::string& key) {
    if (uiSounds_.count(key) && uiSounds_.at(key)) {
        if (Mix_PlayChannel(-1, uiSounds_.at(key).get(), 0) == -1) {
            LOG_ERROR("Mix_PlayChannel Error for " + key + ": " + std::string(Mix_GetError()));
        } else {
            LOG_DEBUG("Playing UI sound: " + key);
        }
    } else {
        LOG_ERROR("UI Sound '" + key + "' not found or not loaded");
    }
}

void PulseAudioPlayer::playAmbienceMusic(const std::string& path) {
    LOG_DEBUG("Attempting to play ambience music: " + path);
    if (path.empty() || !std::filesystem::exists(path) || !std::filesystem::is_regular_file(path)) {
        LOG_INFO("Invalid ambience music path: " + path);
        stopMusic();
        ambienceMusic_.reset();
        cachedAmbiencePath_ = "";
        currentPlayingMusicType_ = MusicType::None;
        return;
    }
    stopMusic();
    if (cachedAmbiencePath_ == path && ambienceMusic_) {
        LOG_DEBUG("Reusing cached ambience music: " + path);
    } else {
        ambienceMusic_.reset(Mix_LoadMUS(path.c_str()));
        if (!ambienceMusic_) {
            LOG_ERROR("Mix_LoadMUS Error for ambience music " + path + ": " + std::string(Mix_GetError()));
            currentPlayingMusicType_ = MusicType::None;
            return;
        }
        cachedAmbiencePath_ = path;
        LOG_DEBUG("Ambience music loaded and cached from " + path);
    }
    if (Mix_PlayMusic(ambienceMusic_.get(), -1) == -1) {
        LOG_ERROR("Mix_PlayMusic Error for ambience music " + path + ": " + std::string(Mix_GetError()));
        currentPlayingMusicType_ = MusicType::None;
    } else {
        currentPlayingMusicType_ = MusicType::Ambience;
        if (ambienceMusic_) {
            double duration = Mix_MusicDuration(ambienceMusic_.get());
            if (duration >= 0.0) {
                dist_ = std::uniform_real_distribution<double>(0.0, duration);
                double randomPosition = dist_(rng_);
                if (Mix_SetMusicPosition(randomPosition) == -1) {
                    LOG_ERROR("Mix_SetMusicPosition Error for ambience music " + path + ": " + std::string(Mix_GetError()));
                }
            } else {
                LOG_ERROR("Could not get duration for ambience music " + path + ". Playing from beginning.");
            }
        }
        applyAudioSettings();
    }
}

void PulseAudioPlayer::playTableMusic(const std::string& path) {
    LOG_DEBUG("Attempting to play table music: " + path + ", current cache: " + cachedTableMusicPath_ + ", tableMusic valid: " + (tableMusic_ ? "yes" : "no"));
    if (path.empty() || !std::filesystem::exists(path) || !std::filesystem::is_regular_file(path)) {
        LOG_WARN("Table has no custom music, playing ambience.");
        stopMusic();
        currentPlayingMusicType_ = MusicType::None;
        if (!settings_.ambienceSound.empty() && std::filesystem::exists(settings_.ambienceSound) && std::filesystem::is_regular_file(settings_.ambienceSound)) {
            playAmbienceMusic(settings_.ambienceSound);
        }
        return;
    }
    if (cachedTableMusicPath_ == path && tableMusic_ && Mix_MusicDuration(tableMusic_.get()) > 0.0) {
        LOG_DEBUG("Reusing cached table music: " + path);
        stopMusic();
    } else {
        LOG_DEBUG("Loading new table music: " + path + " (cache path: " + cachedTableMusicPath_ + ", tableMusic valid: " + (tableMusic_ ? "yes" : "no") + ")");
        stopMusic();
        tableMusic_.reset(Mix_LoadMUS(path.c_str()));
        if (!tableMusic_) {
            LOG_ERROR("Mix_LoadMUS Error for table music " + path + ": " + std::string(Mix_GetError()));
            currentPlayingMusicType_ = MusicType::None;
            cachedTableMusicPath_ = "";
            return;
        }
        cachedTableMusicPath_ = path;
        LOG_DEBUG("Table music loaded and cached from " + path);
    }
    if (Mix_PlayMusic(tableMusic_.get(), -1) == -1) {
        LOG_ERROR("Mix_PlayMusic Error for table music " + path + ": " + std::string(Mix_GetError()));
        currentPlayingMusicType_ = MusicType::None;
    } else {
        currentPlayingMusicType_ = MusicType::Table;
        applyAudioSettings();
        LOG_DEBUG("Successfully playing table music: " + path);
    }
}

void PulseAudioPlayer::playCustomLaunch(const std::string& path) {
    LOG_DEBUG("Attempting to play custom launch: " + path);
    if (path.empty() || !std::filesystem::exists(path) || !std::filesystem::is_regular_file(path)) {
        LOG_INFO("Invalid custom launch path: " + path);
        launchAudio_.reset();
        cachedLaunchAudioPath_ = "";
        currentPlayingMusicType_ = MusicType::None;
        return;
    }
    if (cachedLaunchAudioPath_ == path && launchAudio_) {
        LOG_DEBUG("Reusing cached launch audio: " + path);
    } else {
        launchAudio_.reset(Mix_LoadMUS(path.c_str()));
        if (!launchAudio_) {
            LOG_ERROR("Mix_LoadMUS Error for launch audio " + path + ": " + std::string(Mix_GetError()));
            currentPlayingMusicType_ = MusicType::None;
            return;
        }
        cachedLaunchAudioPath_ = path;
        LOG_DEBUG("Launch audio loaded and cached from " + path);
    }
    if (Mix_PlayMusic(launchAudio_.get(), 0) == -1) {
        LOG_ERROR("Mix_PlayMusic Error for launch audio " + path + ": " + std::string(Mix_GetError()));
        currentPlayingMusicType_ = MusicType::None;
    } else {
        currentPlayingMusicType_ = MusicType::Launch;
        applyAudioSettings();
    }
}

void PulseAudioPlayer::stopMusic() {
    if (Mix_PlayingMusic()) {
        Mix_HaltMusic();
        LOG_DEBUG("Halted current background music.");
    }
    currentPlayingMusicType_ = MusicType::None;
}

void PulseAudioPlayer::applyAudioSettings() {
    bool effective_ui_mute = settings_.masterMute || settings_.interfaceAudioMute;
    float effective_ui_vol = (settings_.interfaceAudioVol / 100.0f) * (settings_.masterVol / 100.0f);
    int uiVolume = effective_ui_mute ? 0 : static_cast<int>(effective_ui_vol * MIX_MAX_VOLUME);
    Mix_Volume(-1, uiVolume);
    LOG_DEBUG("UI sounds volume set to " + (effective_ui_mute ? "muted" : std::to_string(effective_ui_vol * 100.0f) + "%") + " (SDL_mixer: " + std::to_string(uiVolume) + ")");
    if (Mix_PlayingMusic()) {
        if (currentPlayingMusicType_ == MusicType::Ambience) {
            bool effective_ambience_mute = settings_.masterMute || settings_.interfaceAmbienceMute;
            float effective_ambience_vol = (settings_.interfaceAmbienceVol / 100.0f) * (settings_.masterVol / 100.0f);
            int musicVolume = effective_ambience_mute ? 0 : static_cast<int>(effective_ambience_vol * MIX_MAX_VOLUME);
            Mix_VolumeMusic(musicVolume);
            LOG_DEBUG("Ambience music volume set to " + (effective_ambience_mute ? "muted" : std::to_string(effective_ambience_vol * 100.0f) + "%") + " (SDL_mixer: " + std::to_string(musicVolume) + ")");
        } else if (currentPlayingMusicType_ == MusicType::Table) {
            bool effective_table_mute = settings_.masterMute || settings_.tableMusicMute;
            float effective_table_vol = (settings_.tableMusicVol / 100.0f) * (settings_.masterVol / 100.0f);
            int musicVolume = effective_table_mute ? 0 : static_cast<int>(effective_table_vol * MIX_MAX_VOLUME);
            Mix_VolumeMusic(musicVolume);
            LOG_DEBUG("Table music volume set to " + (effective_table_mute ? "muted" : std::to_string(effective_table_vol * 100.0f) + "%") + " (SDL_mixer: " + std::to_string(musicVolume) + ")");
        } else if (currentPlayingMusicType_ == MusicType::Launch) {
            bool effective_table_mute = settings_.masterMute || settings_.interfaceAudioMute;
            float effective_table_vol = (settings_.interfaceAudioVol / 100.0f) * (settings_.masterVol / 100.0f);
            int musicVolume = effective_table_mute ? 0 : static_cast<int>(effective_table_vol * MIX_MAX_VOLUME);
            Mix_VolumeMusic(musicVolume);
            LOG_DEBUG("Launch Audio volume set to " + (effective_table_mute ? "muted" : std::to_string(effective_table_vol * 100.0f) + "%") + " (SDL_mixer: " + std::to_string(musicVolume) + ")");
        } else {
            Mix_VolumeMusic(0);
            LOG_DEBUG("Unknown music playing, setting music volume to 0.");
        }
    } else {
        Mix_VolumeMusic(0);
        LOG_DEBUG("No music playing, setting music volume to 0.");
    }
}

void PulseAudioPlayer::updateSettings(const Settings& newSettings) {
    LOG_DEBUG("Updating PulseAudioPlayer settings.");
    bool uiSoundPathsChanged = (settings_.panelToggleSound != newSettings.panelToggleSound) ||
                               (settings_.scrollNormalSound != newSettings.scrollNormalSound) ||
                               (settings_.scrollFastSound != newSettings.scrollFastSound) ||
                               (settings_.scrollJumpSound != newSettings.scrollJumpSound) ||
                               (settings_.scrollRandomSound != newSettings.scrollRandomSound) ||
                               (settings_.launchTableSound != newSettings.launchTableSound) ||
                               (settings_.launchScreenshotSound != newSettings.launchScreenshotSound) ||
                               (settings_.screenshotTakeSound != newSettings.screenshotTakeSound);
    bool ambiencePathChanged = (settings_.ambienceSound != newSettings.ambienceSound);
    settings_ = newSettings;
    if (uiSoundPathsChanged) {
        LOG_DEBUG("UI sound paths changed, reloading UI sounds.");
        loadSounds();
    }
    if (ambiencePathChanged && currentPlayingMusicType_ == MusicType::Ambience) {
        LOG_DEBUG("Ambience music path changed, attempting to restart ambience.");
        playAmbienceMusic(settings_.ambienceSound);
    } else if (currentPlayingMusicType_ == MusicType::None && !settings_.ambienceSound.empty() &&
               std::filesystem::exists(settings_.ambienceSound) &&
               std::filesystem::is_regular_file(settings_.ambienceSound)) {
        LOG_DEBUG("Ambience music was not playing but has a valid path, attempting to start.");
        playAmbienceMusic(settings_.ambienceSound);
    } else if (currentPlayingMusicType_ == MusicType::Ambience) {
        applyAudioSettings();
    }
    applyAudioSettings();
}

std::string PulseAudioPlayer::trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\n\r\f\v");
    if (std::string::npos == first) {
        return str;
    }
    size_t last = str.find_last_not_of(" \t\n\r\f\v");
    return str.substr(first, (last - first + 1));
}