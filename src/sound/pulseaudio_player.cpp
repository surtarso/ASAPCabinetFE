#include "sound/pulseaudio_player.h"
#include "utils/logging.h"
#include <algorithm> // For std::find_if
#include <iostream>
#include <filesystem> // For std::filesystem::exists and is_regular_file
#include <cctype>     // For std::isspace
#include <thread> // Add for std::this_thread::sleep_for
#include <chrono> // Add for std::chrono::milliseconds

// Flag to track audio initialization
static bool audio_initialized = false;

PulseAudioPlayer::PulseAudioPlayer(const std::string& exeDir, const Settings& settings)
    : exeDir_(exeDir),
      settings_(settings),
      currentPlayingMusicType_(MusicType::None),
      ambienceMusic_(nullptr, Mix_FreeMusic),
      tableMusic_(nullptr, Mix_FreeMusic),
      launchAudio_(nullptr, Mix_FreeMusic),
      rng_(std::chrono::steady_clock::now().time_since_epoch().count()),
      dist_(0.0, 1.0) {
    if (!audio_initialized) {
        // Initialize SDL_mixer
        if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
            LOG_ERROR("PulseAudioPlayer: Mix_OpenAudio failed: " << Mix_GetError());
            throw std::runtime_error("Failed to initialize audio");
        }
        int flags = MIX_INIT_MP3 | MIX_INIT_OGG;
        if (Mix_Init(flags) != flags) {
            LOG_ERROR("PulseAudioPlayer: Mix_Init failed: " << Mix_GetError());
            Mix_CloseAudio();
            throw std::runtime_error("PulseAudioPlayer: Failed to initialize MP3/OGG support");
        }
        audio_initialized = true;
        LOG_DEBUG("PulseAudioPlayer: SDL_mixer initialized with MP3 and OGG support");
    } else {
        LOG_DEBUG("PulseAudioPlayer: SDL_mixer already initialized");
    }
    // Initialize the UI sounds map with placeholders (will be loaded in loadSounds)
    // Using Mix_Chunk for UI sounds as they are short effects and can play concurrently
    uiSounds_.emplace("config_toggle", std::unique_ptr<Mix_Chunk, void(*)(Mix_Chunk*)>(nullptr, Mix_FreeChunk));
    uiSounds_.emplace("scroll_prev", std::unique_ptr<Mix_Chunk, void(*)(Mix_Chunk*)>(nullptr, Mix_FreeChunk));
    uiSounds_.emplace("scroll_next", std::unique_ptr<Mix_Chunk, void(*)(Mix_Chunk*)>(nullptr, Mix_FreeChunk));
    uiSounds_.emplace("scroll_fast_prev", std::unique_ptr<Mix_Chunk, void(*)(Mix_Chunk*)>(nullptr, Mix_FreeChunk));
    uiSounds_.emplace("scroll_fast_next", std::unique_ptr<Mix_Chunk, void(*)(Mix_Chunk*)>(nullptr, Mix_FreeChunk));
    uiSounds_.emplace("scroll_jump_prev", std::unique_ptr<Mix_Chunk, void(*)(Mix_Chunk*)>(nullptr, Mix_FreeChunk));
    uiSounds_.emplace("scroll_jump_next", std::unique_ptr<Mix_Chunk, void(*)(Mix_Chunk*)>(nullptr, Mix_FreeChunk));
    uiSounds_.emplace("scroll_random", std::unique_ptr<Mix_Chunk, void(*)(Mix_Chunk*)>(nullptr, Mix_FreeChunk));
    uiSounds_.emplace("launch_table", std::unique_ptr<Mix_Chunk, void(*)(Mix_Chunk*)>(nullptr, Mix_FreeChunk));
    uiSounds_.emplace("launch_screenshot", std::unique_ptr<Mix_Chunk, void(*)(Mix_Chunk*)>(nullptr, Mix_FreeChunk));
    uiSounds_.emplace("config_save", std::unique_ptr<Mix_Chunk, void(*)(Mix_Chunk*)>(nullptr, Mix_FreeChunk));
    uiSounds_.emplace("screenshot_take", std::unique_ptr<Mix_Chunk, void(*)(Mix_Chunk*)>(nullptr, Mix_FreeChunk));
    uiSounds_.emplace("screenshot_quit", std::unique_ptr<Mix_Chunk, void(*)(Mix_Chunk*)>(nullptr, Mix_FreeChunk));
}

PulseAudioPlayer::~PulseAudioPlayer() {
    stopMusic(); // Stop any playing music
    Mix_HaltChannel(-1); // Stop all channels for UI sounds
    uiSounds_.clear(); // unique_ptr handles Mix_FreeChunk
    ambienceMusic_.reset(); // unique_ptr handles Mix_FreeMusic
    tableMusic_.reset(); // unique_ptr handles Mix_FreeMusic
    launchAudio_.reset(); // unique_ptr handles Mix_FreeMusic
    std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Allow PulseAudio to process
    if (audio_initialized) {
        while (Mix_Init(0)) { // Close any additional SDL_mixer initializations
            Mix_Quit();
        }
        Mix_CloseAudio(); // Close the audio device
        Mix_Quit(); // Final cleanup of SDL_mixer
        audio_initialized = false;
        LOG_DEBUG("PulseAudioPlayer: PulseAudioPlayer destroyed and SDL_mixer quit");
    }
}

/**
 * @brief Loads all necessary sound resources based on current settings.
 * This includes UI sound effects and the ambience music file.
 */
void PulseAudioPlayer::loadSounds() {
    LOG_DEBUG("PulseAudioPlayer: Loading sounds...");

    // Helper to load UI sounds (Mix_Chunk)
    auto loadUiSound = [&](const std::string& key, const std::string& path) {
        if (path.empty()) {
            LOG_DEBUG("PulseAudioPlayer: UI sound path is empty for key: " << key);
            uiSounds_.at(key).reset(); // Ensure it's null if path is empty
            return;
        }
        std::string fullPath = exeDir_ + path; // trim will be applied inside config service
        if (std::filesystem::exists(fullPath) && std::filesystem::is_regular_file(fullPath)) {
            uiSounds_.at(key).reset(Mix_LoadWAV(fullPath.c_str()));
            if (!uiSounds_.at(key)) {
                LOG_ERROR("PulseAudioPlayer: Mix_LoadWAV Error for " << key << " at " << fullPath << ": " << Mix_GetError());
            }
            // else {
            //     LOG_DEBUG("PulseAudioPlayer: UI sound '" << key << "' loaded successfully from " << fullPath);
            // }
        } else {
            LOG_ERROR("PulseAudioPlayer: UI sound file not found or not a regular file for " << key << " at " << fullPath);
            uiSounds_.at(key).reset(); // Ensure it's null if file is not found/invalid
        }
    };

    // Load all UI sounds
    loadUiSound("config_toggle", settings_.configToggleSound);
    loadUiSound("scroll_prev", settings_.scrollPrevSound);
    loadUiSound("scroll_next", settings_.scrollNextSound);
    loadUiSound("scroll_fast_prev", settings_.scrollFastPrevSound);
    loadUiSound("scroll_fast_next", settings_.scrollFastNextSound);
    loadUiSound("scroll_jump_prev", settings_.scrollJumpPrevSound);
    loadUiSound("scroll_jump_next", settings_.scrollJumpNextSound);
    loadUiSound("scroll_random", settings_.scrollRandomSound);
    loadUiSound("launch_table", settings_.launchTableSound);
    loadUiSound("launch_screenshot", settings_.launchScreenshotSound);
    loadUiSound("config_save", settings_.configSaveSound);
    loadUiSound("screenshot_take", settings_.screenshotTakeSound);
    loadUiSound("screenshot_quit", settings_.screenshotQuitSound);

    // Load ambience sound (only if path is valid and not empty)
    if (settings_.ambienceSound.empty()) {
        LOG_INFO("PulseAudioPlayer: Ambience sound path is empty in settings. Ambience will not play.");
        ambienceMusic_.reset(); // Ensure it's null
    } else {
        std::string ambienceFullPath = exeDir_ + settings_.ambienceSound;
        if (std::filesystem::exists(ambienceFullPath) && std::filesystem::is_regular_file(ambienceFullPath)) {
            ambienceMusic_.reset(Mix_LoadMUS(ambienceFullPath.c_str()));
            if (!ambienceMusic_) {
                LOG_ERROR("PulseAudioPlayer: Mix_LoadMUS Error for ambience at " << ambienceFullPath << ": " << Mix_GetError());
            } else {
                LOG_DEBUG("PulseAudioPlayer: Ambience sound loaded successfully from " << ambienceFullPath);
            }
        } else {
            LOG_INFO("PulseAudioPlayer: Ambience sound file not found or is not a regular file at " << ambienceFullPath << ". Ambience will not play.");
            ambienceMusic_.reset(); // Ensure it's null if file doesn't exist or is a directory
        }
    }
}

/**
 * @brief Plays a specific UI sound effect.
 * @param key The unique identifier for the UI sound.
 */
void PulseAudioPlayer::playUISound(const std::string& key) {
    if (uiSounds_.count(key) && uiSounds_.at(key)) {
        // Play Mix_Chunk on an available channel (-1 for first available), 0 times (play once)
        if (Mix_PlayChannel(-1, uiSounds_.at(key).get(), 0) == -1) {
            LOG_ERROR("PulseAudioPlayer: Mix_PlayChannel Error for " << key << ": " << Mix_GetError());
        } else {
            LOG_DEBUG("PulseAudioPlayer: Playing UI sound: " << key);
        }
    } else {
        LOG_ERROR("PulseAudioPlayer: UI Sound '" << key << "' not found or not loaded");
    }
}

/**
 * @brief Plays the background ambience music.
 * @param path The full path to the ambience music file.
 */
void PulseAudioPlayer::playAmbienceMusic(const std::string& path) {
    LOG_DEBUG("PulseAudioPlayer: Attempting to play ambience music: " << path);

    stopMusic(); // Stop any currently playing music (table or previous ambience)

    if (path.empty() || !std::filesystem::exists(path) || !std::filesystem::is_regular_file(path)) {
        LOG_INFO("PulseAudioPlayer: No ambience music path provided, file not found, or not a regular file: " << path);
        ambienceMusic_.reset(); // Ensure it's null if path is invalid
        currentPlayingMusicType_ = MusicType::None; // Update state
        return; // Nothing to play
    }

    // Load music if not already loaded or if path changed
    // Check if the currently loaded ambience music is different from the requested path
    // Mix_GetMusicTitle might return nullptr if no music is loaded or if it's not a file-based music
    bool needsReload = !ambienceMusic_ ||
                       (Mix_GetMusicTitle(ambienceMusic_.get()) == nullptr) ||
                       (std::string(Mix_GetMusicTitle(ambienceMusic_.get())) != path);

    if (needsReload) {
        ambienceMusic_.reset(Mix_LoadMUS(path.c_str()));
        if (!ambienceMusic_) {
            LOG_ERROR("PulseAudioPlayer: Mix_LoadMUS Error for ambience music " << path << ": " << Mix_GetError());
            currentPlayingMusicType_ = MusicType::None; // Update state
            return;
        }
        LOG_DEBUG("PulseAudioPlayer: Ambience music loaded successfully from " << path);
    }

    // Play ambience on loop (-1)
    if (Mix_PlayMusic(ambienceMusic_.get(), -1) == -1) {
        LOG_ERROR("PulseAudioPlayer: Mix_PlayMusic Error for ambience music " << path << ": " << Mix_GetError());
        currentPlayingMusicType_ = MusicType::None; // Update state on failure
    } else {
        //LOG_INFO("PulseAudioPlayer: Playing ambience music: " << path);
        currentPlayingMusicType_ = MusicType::Ambience; // Update state on success
    }

    if (ambienceMusic_) {
        double duration = Mix_MusicDuration(ambienceMusic_.get());
        if (duration >= 0.0) { // Check if duration is valid
            // Set the distribution to the actual duration of the music
            dist_ = std::uniform_real_distribution<double>(0.0, duration);
            double randomPosition = dist_(rng_); // Generate random position within duration

            if (Mix_SetMusicPosition(randomPosition) == -1) {
                LOG_ERROR("PulseAudioPlayer: Mix_SetMusicPosition Error for ambience music " << path << ": " << Mix_GetError());
            } 
            // else {
                // LOG_DEBUG("PulseAudioPlayer: Ambience music set to random position: " << randomPosition << " seconds.");
            // }
        } else {
            LOG_ERROR("PulseAudioPlayer: Could not get duration for ambience music " << path << ". Playing from beginning.");
        }
    }

    applyAudioSettings(); // Reapply volume after playing
}

/**
 * @brief Plays the table-specific music.
 * @param path The full path to the table music file.
 */
void PulseAudioPlayer::playTableMusic(const std::string& path) {
    LOG_DEBUG("PulseAudioPlayer: Attempting to play table music: " << path);

    stopMusic(); // Stop any currently playing music (ambience or previous table)

    if (path.empty() || !std::filesystem::exists(path) || !std::filesystem::is_regular_file(path)) {
        //LOG_INFO("PulseAudioPlayer: No table music path provided, file not found, or not a regular file: " << path);
        tableMusic_.reset(); // Ensure it's null if path is invalid
        currentPlayingMusicType_ = MusicType::None; // Update state
        // If no table music, try to resume ambience
        if (!settings_.ambienceSound.empty() && std::filesystem::exists(exeDir_ + settings_.ambienceSound) && std::filesystem::is_regular_file(exeDir_ + settings_.ambienceSound)) {
             playAmbienceMusic(exeDir_ + settings_.ambienceSound);
        }
        return; // Nothing more to play for table music
    }

    // Load music if not already loaded or if path changed
    bool needsReload = !tableMusic_ ||
                       (Mix_GetMusicTitle(tableMusic_.get()) == nullptr) ||
                       (std::string(Mix_GetMusicTitle(tableMusic_.get())) != path);

    if (needsReload) {
        tableMusic_.reset(Mix_LoadMUS(path.c_str()));
        if (!tableMusic_) {
            LOG_ERROR("PulseAudioPlayer: Mix_LoadMUS Error for table music " << path << ": " << Mix_GetError());
            currentPlayingMusicType_ = MusicType::None; // Update state
            return;
        }
        LOG_DEBUG("PulseAudioPlayer: Table music loaded successfully from " << path);
    }

    // Play table music on loop (-1)
    if (Mix_PlayMusic(tableMusic_.get(), -1) == -1) {
        LOG_ERROR("PulseAudioPlayer: Mix_PlayMusic Error for table music " << path << ": " << Mix_GetError());
        currentPlayingMusicType_ = MusicType::None; // Update state on failure
    } else {
        //LOG_INFO("PulseAudioPlayer: Playing table music: " << path);
        currentPlayingMusicType_ = MusicType::Table; // Update state on success
    }
    applyAudioSettings(); // Reapply volume after playing
}

/**
 * @brief Plays the table-specific music.
 * @param path The full path to the table music file.
 */
void PulseAudioPlayer::playCustomLaunch(const std::string& path) {
    LOG_DEBUG("PulseAudioPlayer: Attempting to play table custom launch: " << path);

    if (path.empty() || !std::filesystem::exists(path) || !std::filesystem::is_regular_file(path)) {
        LOG_INFO("PulseAudioPlayer: No custom launch path provided, file not found, or not a regular file: " << path);
        launchAudio_.reset(); // Ensure it's null if path is invalid
        currentPlayingMusicType_ = MusicType::None; // Update state
        return;
    }

    // Load launch audio if not already loaded or if path changed
    bool needsReload = !launchAudio_ ||
                       (Mix_GetMusicTitle(launchAudio_.get()) == nullptr) ||
                       (std::string(Mix_GetMusicTitle(launchAudio_.get())) != path);

    if (needsReload) {
        launchAudio_.reset(Mix_LoadMUS(path.c_str()));
        if (!launchAudio_) {
            LOG_ERROR("PulseAudioPlayer: Mix_LoadMUS Error for launch audio " << path << ": " << Mix_GetError());
            currentPlayingMusicType_ = MusicType::None; // Update state
            return;
        }
        LOG_DEBUG("PulseAudioPlayer: Launch audio (custom) loaded successfully from " << path);
        currentPlayingMusicType_ = MusicType::Launch;
    }
    Mix_PlayMusic(launchAudio_.get(), 0);
    applyAudioSettings(); // Reapply volume after playing
}

/**
 * @brief Stops any currently playing background music (ambience or table music).
 */
void PulseAudioPlayer::stopMusic() {
    if (Mix_PlayingMusic()) {
        Mix_HaltMusic(); // Stops any playing music on the music channel
        LOG_DEBUG("PulseAudioPlayer: Halted current background music.");
    }
    currentPlayingMusicType_ = MusicType::None; // Update state
}

/**
 * @brief Applies the current audio settings (volumes, mute states) to all active sound types.
 */
void PulseAudioPlayer::applyAudioSettings() {
    // Apply master mute and volume to UI sounds
    bool effective_ui_mute = settings_.masterMute || settings_.interfaceAudioMute;
    float effective_ui_vol = (settings_.interfaceAudioVol / 100.0f) * (settings_.masterVol / 100.0f);
    int uiVolume = effective_ui_mute ? 0 : static_cast<int>(effective_ui_vol * MIX_MAX_VOLUME);
    Mix_Volume(-1, uiVolume);
    LOG_DEBUG("PulseAudioPlayer: UI sounds volume set to " << (effective_ui_mute ? "muted" : std::to_string(effective_ui_vol * 100.0f) + "%") << " (SDL_mixer: " << uiVolume << ")");

    // Apply master mute and volume to music
    if (Mix_PlayingMusic()) {
        if (currentPlayingMusicType_ == MusicType::Ambience) {
            bool effective_ambience_mute = settings_.masterMute || settings_.interfaceAmbienceMute;
            float effective_ambience_vol = (settings_.interfaceAmbienceVol / 100.0f) * (settings_.masterVol / 100.0f);
            int musicVolume = effective_ambience_mute ? 0 : static_cast<int>(effective_ambience_vol * MIX_MAX_VOLUME);
            Mix_VolumeMusic(musicVolume);
            LOG_DEBUG("PulseAudioPlayer: Ambience music volume set to " << (effective_ambience_mute ? "muted" : std::to_string(effective_ambience_vol * 100.0f) + "%") << " (SDL_mixer: " << musicVolume << ")");
        } else if (currentPlayingMusicType_ == MusicType::Table) {
            bool effective_table_mute = settings_.masterMute || settings_.tableMusicMute;
            float effective_table_vol = (settings_.tableMusicVol / 100.0f) * (settings_.masterVol / 100.0f);
            int musicVolume = effective_table_mute ? 0 : static_cast<int>(effective_table_vol * MIX_MAX_VOLUME);
            Mix_VolumeMusic(musicVolume);
            LOG_DEBUG("PulseAudioPlayer: Table music volume set to " << (effective_table_mute ? "muted" : std::to_string(effective_table_vol * 100.0f) + "%") << " (SDL_mixer: " << musicVolume << ")");
        } else if (currentPlayingMusicType_ == MusicType::Launch) {
            bool effective_table_mute = settings_.masterMute || settings_.interfaceAudioMute;
            float effective_table_vol = (settings_.interfaceAudioVol / 100.0f) * (settings_.masterVol / 100.0f);
            int musicVolume = effective_table_mute ? 0 : static_cast<int>(effective_table_vol * MIX_MAX_VOLUME);
            Mix_VolumeMusic(musicVolume);
            LOG_DEBUG("PulseAudioPlayer: Launch Audio volume set to " << (effective_table_mute ? "muted" : std::to_string(effective_table_vol * 100.0f) + "%") << " (SDL_mixer: " << musicVolume << ")");
        } else {
            Mix_VolumeMusic(0);
            LOG_DEBUG("PulseAudioPlayer: Unknown music playing, setting music volume to 0.");
        }
    } else {
        Mix_VolumeMusic(0);
        LOG_DEBUG("PulseAudioPlayer: No music playing, setting music volume to 0.");
    }
}

/**
 * @brief Updates the internal settings and reloads sounds if necessary.
 * @param newSettings The new settings to apply.
 */
void PulseAudioPlayer::updateSettings(const Settings& newSettings) {
    LOG_DEBUG("PulseAudioPlayer: Updating PulseAudioPlayer settings.");

    // Check if UI sound paths have changed to trigger a reload of Mix_Chunk assets
    bool uiSoundPathsChanged = (settings_.configToggleSound != newSettings.configToggleSound) ||
                               (settings_.scrollPrevSound != newSettings.scrollPrevSound) ||
                               (settings_.scrollNextSound != newSettings.scrollNextSound) ||
                               (settings_.scrollFastPrevSound != newSettings.scrollFastPrevSound) ||
                               (settings_.scrollFastNextSound != newSettings.scrollFastNextSound) ||
                               (settings_.scrollJumpPrevSound != newSettings.scrollJumpPrevSound) ||
                               (settings_.scrollJumpNextSound != newSettings.scrollJumpNextSound) ||
                               (settings_.scrollRandomSound != newSettings.scrollRandomSound) ||
                               (settings_.launchTableSound != newSettings.launchTableSound) ||
                               (settings_.launchScreenshotSound != newSettings.launchScreenshotSound) ||
                               (settings_.configSaveSound != newSettings.configSaveSound) ||
                               (settings_.screenshotTakeSound != newSettings.screenshotTakeSound) ||
                               (settings_.screenshotQuitSound != newSettings.screenshotQuitSound);

    // Check if ambience music path has changed
    bool ambiencePathChanged = (settings_.ambienceSound != newSettings.ambienceSound);

    // Update the internal settings_ member variable with the new settings
    settings_ = newSettings;

    if (uiSoundPathsChanged) {
        LOG_DEBUG("PulseAudioPlayer: UI sound paths changed, reloading UI sounds.");
        // Reload all UI sounds (Mix_Chunk)
        loadSounds();
    }

    // Handle ambience music: if path changed, restart it. If it was playing and path didn't change,
    // just re-apply settings. If it wasn't playing but now has a valid path, start it.
    if (ambiencePathChanged) {
        LOG_DEBUG("PulseAudioPlayer: Ambience music path changed, attempting to restart ambience.");
        playAmbienceMusic(exeDir_ + settings_.ambienceSound);
    } else if (currentPlayingMusicType_ == MusicType::Ambience) {
        // Ambience was playing and path didn't change, just re-apply volume/mute
        applyAudioSettings();
    } else if (currentPlayingMusicType_ == MusicType::None && !settings_.ambienceSound.empty() &&
               std::filesystem::exists(exeDir_ + settings_.ambienceSound) &&
               std::filesystem::is_regular_file(exeDir_ + settings_.ambienceSound)) {
        LOG_DEBUG("PulseAudioPlayer: Ambience music was not playing but has a valid path, attempting to start.");
        playAmbienceMusic(exeDir_ + settings_.ambienceSound);
    } else {
        // If ambience path is empty or invalid, and it was previously playing ambience, ensure it's stopped
        if (currentPlayingMusicType_ == MusicType::Ambience) {
            stopMusic();
        }
        ambienceMusic_.reset();
    }

    // Always apply audio settings to update volumes/mutes for all categories
    applyAudioSettings();
}

/**
 * @brief Trims leading and trailing whitespace from a string.
 * @param str The string to trim.
 * @return The trimmed string.
 */
std::string PulseAudioPlayer::trim(const std::string& str) {
    // Find the first non-whitespace character
    size_t first = str.find_first_not_of(" \t\n\r\f\v");
    if (std::string::npos == first) {
        return str; // No non-whitespace characters, return original string (might be empty or all spaces)
    }
    // Find the last non-whitespace character
    size_t last = str.find_last_not_of(" \t\n\r\f\v");
    return str.substr(first, (last - first + 1));
}
