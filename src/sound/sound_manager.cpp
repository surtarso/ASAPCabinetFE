#include "sound/sound_manager.h"
#include "utils/logging.h"
#include <algorithm> // For std::find_if
#include <iostream>
#include <filesystem> // For std::filesystem::exists and is_regular_file
#include <cctype>     // For std::isspace

// Assuming Settings struct is defined somewhere and accessible
// #include "config/settings.h" // Example path if settings is in a separate header

SoundManager::SoundManager(const std::string& exeDir, const Settings& settings)
    : exeDir_(exeDir),
      settings_(settings), // Initialize settings_ with passed settings
      ambienceMusic_(nullptr, Mix_FreeMusic), // Initialize smart pointers for music
      tableMusic_(nullptr, Mix_FreeChunk), // Initialize tableMusic_ as Mix_Chunk
      tableMusicChannel_(-1), // Initialize channel to an invalid value
      isAmbiencePlaying_(false), // Initialize independent playing state
      isTableMusicPlaying_(false) // Initialize independent playing state
{
    // Initialize SDL_mixer
    // Support MP3 and OGG (commonly used for music)
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
        LOG_ERROR("SoundManager: Mix_OpenAudio failed: " << Mix_GetError());
        throw std::runtime_error("Failed to initialize audio");
    }
    int flags = MIX_INIT_MP3 | MIX_INIT_OGG; // Add OGG support
    if (Mix_Init(flags) != flags) {
        LOG_ERROR("SoundManager: Mix_Init failed: " << Mix_GetError());
        Mix_CloseAudio();
        throw std::runtime_error("SoundManager: Failed to initialize MP3/OGG support");
    }
    LOG_DEBUG("SoundManager: SDL_mixer initialized with MP3 and OGG support");

    // Allocate a specific channel for table music
    // We request more channels than default (8) if needed.
    Mix_AllocateChannels(16); // Ensure enough channels are available (default is 8 for effects)
    tableMusicChannel_ = Mix_GroupAvailable(-1); // Get first available channel not in use
    if (tableMusicChannel_ == -1) {
        LOG_ERROR("SoundManager: Could not allocate a channel for table music. Table music may not play concurrently.");
        // Decide how to handle this critical error: throw, or proceed without table music
    } else {
        LOG_DEBUG("SoundManager: Allocated channel " << tableMusicChannel_ << " for table music.");
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

SoundManager::~SoundManager() {
    uiSounds_.clear();  // unique_ptr handles Mix_FreeChunk
    ambienceMusic_.reset(); // unique_ptr handles Mix_FreeMusic
    tableMusic_.reset(); // unique_ptr handles Mix_FreeChunk (now a Mix_Chunk)
    Mix_CloseAudio();
    Mix_Quit();
    LOG_DEBUG("SoundManager: SoundManager destroyed and SDL_mixer quit");
}

/**
 * @brief Helper to load UI sounds (Mix_Chunk).
 * This function is separated for clarity and reusability.
 * @param key The unique identifier for the UI sound.
 * @param path The relative path to the sound file.
 */
void SoundManager::loadUiSound(const std::string& key, const std::string& path) {
    if (path.empty()) {
        LOG_DEBUG("SoundManager: UI sound path is empty for key: " << key);
        uiSounds_.at(key).reset(); // Ensure it's null if path is empty
        return;
    }
    std::string fullPath = exeDir_ + path;
    if (std::filesystem::exists(fullPath) && std::filesystem::is_regular_file(fullPath)) {
        uiSounds_.at(key).reset(Mix_LoadWAV(fullPath.c_str()));
        if (!uiSounds_.at(key)) {
            LOG_ERROR("SoundManager: Mix_LoadWAV Error for " << key << " at " << fullPath << ": " << Mix_GetError());
        }
        // else {
        //     LOG_DEBUG("SoundManager: UI sound '" << key << "' loaded successfully from " << fullPath);
        // }
    } else {
        LOG_ERROR("SoundManager: UI sound file not found or not a regular file for " << key << " at " << fullPath);
        uiSounds_.at(key).reset(); // Ensure it's null if file is not found/invalid
    }
}

/**
 * @brief Loads all necessary sound resources based on current settings.
 * This includes UI sound effects. Ambience and table music are loaded dynamically.
 */
void SoundManager::loadSounds() {
    LOG_DEBUG("SoundManager: Loading sounds...");

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

    // Ambience sound loading happens within playAmbienceMusic or updateSettings
    // to ensure it's playing/reloaded correctly based on application state.
    // We don't load it unconditionally here as it might not be needed immediately or its path could change.
}

/**
 * @brief Plays a specific UI sound effect.
 * @param key The unique identifier for the UI sound.
 */
void SoundManager::playUISound(const std::string& key) {
    if (uiSounds_.count(key) && uiSounds_.at(key)) {
        // Play Mix_Chunk on an available channel (-1 for first available), 0 times (play once)
        if (Mix_PlayChannel(-1, uiSounds_.at(key).get(), 0) == -1) {
            LOG_ERROR("SoundManager: Mix_PlayChannel Error for " << key << ": " << Mix_GetError());
        } else {
            LOG_DEBUG("SoundManager: Playing UI sound: " << key);
        }
    } else {
        LOG_ERROR("SoundManager: UI Sound '" << key << "' not found or not loaded");
    }
}

/**
 * @brief Plays the background ambience music.
 * If the same ambience music is already playing, its volume settings are updated.
 * Otherwise, the new ambience music is loaded and played, persistently looping
 * across table changes unless explicitly stopped or muted.
 * @param path The full path to the ambience music file.
 */
void SoundManager::playAmbienceMusic(const std::string& path) {
    LOG_DEBUG("SoundManager: Attempting to play ambience music: " << path);

    // If no path is provided or it's invalid, stop current ambience.
    if (path.empty() || !std::filesystem::exists(path) || !std::filesystem::is_regular_file(path)) {
        LOG_INFO("SoundManager: No ambience music path provided, file not found, or not a regular file: " << path << ". Stopping ambience.");
        stopAmbienceMusic(); // Use the new stopAmbienceMusic function
        return;
    }

    // Check if the same ambience music is already playing and doesn't need reload.
    // Mix_PlayingMusic() checks if ANY Mix_Music is playing. We need to verify if it's OUR ambience.
    bool currentMusicIsAmbience = Mix_PlayingMusic() && (lastAmbiencePath_ == path);

    if (currentMusicIsAmbience) {
        LOG_DEBUG("SoundManager: Same ambience music (" << path << ") is already playing. Adjusting volume.");
        applyAudioSettings(); // Just reapply settings if it's already playing
        return;
    }

    // If different music is playing or nothing is playing, load and start.
    ambienceMusic_.reset(Mix_LoadMUS(path.c_str()));
    if (!ambienceMusic_) {
        LOG_ERROR("SoundManager: Mix_LoadMUS Error for ambience music " << path << ": " << Mix_GetError());
        isAmbiencePlaying_ = false; // Update state on failure
        lastAmbiencePath_ = ""; // Clear path as it failed to load
        return;
    }
    LOG_DEBUG("SoundManager: Ambience music loaded successfully from " << path);
    lastAmbiencePath_ = path; // Update the path tracker

    // Play ambience on loop (-1)
    if (Mix_PlayMusic(ambienceMusic_.get(), -1) == -1) {
        LOG_ERROR("SoundManager: Mix_PlayMusic Error for ambience music " << path << ": " << Mix_GetError());
        isAmbiencePlaying_ = false; // Update state on failure
    } else {
        LOG_INFO("SoundManager: Playing ambience music: " << path);
        isAmbiencePlaying_ = true; // Update state on success
    }
    applyAudioSettings(); // Reapply volume after playing
}

/**
 * @brief Plays the table-specific music on a dedicated channel.
 * This music plays concurrently with the ambience music. If no valid
 * table music path is provided or the file is not found, any currently
 * playing table music is stopped, and ambience music (if configured)
 * is ensured to be playing.
 * @param path The full path to the table music file.
 */
void SoundManager::playTableMusic(const std::string& path) {
    LOG_DEBUG("SoundManager: Attempting to play table music: " << path);

    // Always stop previous table music before playing a new one.
    stopTableMusic(); // Use the new stopTableMusic function

    if (path.empty() || !std::filesystem::exists(path) || !std::filesystem::is_regular_file(path)) {
        LOG_INFO("SoundManager: No table music path provided, file not found, or not a regular file: " << path << ". Stopping table music.");
        tableMusic_.reset(); // Ensure it's null if path is invalid
        lastTableMusicPath_ = ""; // Clear path tracker
        isTableMusicPlaying_ = false; // Update state

        // If no table music, ensure ambience is playing if configured
        if (!isAmbiencePlaying_ && !settings_.ambienceSound.empty()) {
            playAmbienceMusic(exeDir_ + settings_.ambienceSound);
        }
        return; // Nothing more to play for table music
    }

    // Load table music (now as Mix_Chunk) if path changed or not loaded
    bool needsReload = !tableMusic_ || (lastTableMusicPath_ != path);

    if (needsReload) {
        tableMusic_.reset(Mix_LoadWAV(path.c_str())); // Load as WAV for Mix_Chunk
        if (!tableMusic_) {
            LOG_ERROR("SoundManager: Mix_LoadWAV Error for table music " << path << ": " << Mix_GetError());
            isTableMusicPlaying_ = false; // Update state
            lastTableMusicPath_ = ""; // Clear path tracker
            // If table music failed to load, ensure ambience is playing if configured
            if (!isAmbiencePlaying_ && !settings_.ambienceSound.empty()) {
                playAmbienceMusic(exeDir_ + settings_.ambienceSound);
            }
            return;
        }
        LOG_DEBUG("SoundManager: Table music loaded successfully from " << path);
        lastTableMusicPath_ = path; // Update the path tracker
    }

    // Play table music on its dedicated channel (-1 for infinite loop)
    if (Mix_PlayChannel(tableMusicChannel_, tableMusic_.get(), -1) == -1) {
        LOG_ERROR("SoundManager: Mix_PlayChannel Error for table music " << path << " on channel " << tableMusicChannel_ << ": " << Mix_GetError());
        isTableMusicPlaying_ = false; // Update state on failure
    } else {
        LOG_INFO("SoundManager: Playing table music: " << path << " on channel " << tableMusicChannel_);
        isTableMusicPlaying_ = true; // Update state on success
    }
    applyAudioSettings(); // Reapply volume after playing
}

/**
 * @brief Stops all currently playing music (ambience and table music).
 * This method will halt both Mix_Music and the dedicated Mix_Chunk channel.
 * For more granular control, use stopAmbienceMusic() or stopTableMusic().
 */
void SoundManager::stopMusic() {
    stopAmbienceMusic(); // Stop ambience
    stopTableMusic();   // Stop table music
    LOG_DEBUG("SoundManager: Halted all background music.");
}

/**
 * @brief Stops only the background ambience music.
 * This halts the Mix_Music channel.
 */
void SoundManager::stopAmbienceMusic() {
    if (Mix_PlayingMusic()) { // Checks if any Mix_Music is playing
        Mix_HaltMusic(); // Stops Mix_Music (our ambience)
        LOG_DEBUG("SoundManager: Halted ambience music.");
    }
    isAmbiencePlaying_ = false; // Update state
    lastAmbiencePath_ = ""; // Clear path as it's no longer playing
}

/**
 * @brief Stops only the table-specific music.
 * This halts playback on the dedicated table music channel.
 */
void SoundManager::stopTableMusic() {
    if (tableMusicChannel_ != -1 && Mix_Playing(tableMusicChannel_)) { // Check if channel is valid and playing
        Mix_HaltChannel(tableMusicChannel_); // Stops music on the dedicated channel
        LOG_DEBUG("SoundManager: Halted table music on channel " << tableMusicChannel_ << ".");
    }
    isTableMusicPlaying_ = false; // Update state
    // lastTableMusicPath_ is kept to allow reloading same song without re-reading from disk if needed,
    // but cleared if the path becomes empty in playTableMusic.
}

/**
 * @brief Applies the current audio settings (volumes, mute states) to all active sound types.
 */
void SoundManager::applyAudioSettings() {
    // UI Sounds (Mix_Chunk)
    LOG_DEBUG("SoundManager: Applying UI audio settings. Mute: " << settings_.interfaceAudioMute << ", Volume: " << settings_.interfaceAudioVol);
    int uiVolume = static_cast<int>(settings_.interfaceAudioVol * MIX_MAX_VOLUME / 100.0f);
    if (settings_.interfaceAudioMute) {
        Mix_Volume(-1, 0); // Mute all channels for UI sounds (-1 sets all allocated channels)
        LOG_DEBUG("SoundManager: UI sounds muted.");
    } else {
        Mix_Volume(-1, uiVolume); // Set global volume for all channels
        LOG_DEBUG("SoundManager: UI sounds volume set to " << settings_.interfaceAudioVol << "% (SDL_mixer: " << uiVolume << ")");
    }

    // Ambience Music (Mix_Music)
    LOG_DEBUG("SoundManager: Applying ambience music settings. Mute: " << settings_.interfaceAmbienceMute << ", Volume: " << settings_.interfaceAmbienceVol);
    int ambienceVolume = static_cast<int>(settings_.interfaceAmbienceVol * MIX_MAX_VOLUME / 100.0f);
    if (settings_.interfaceAmbienceMute) {
        Mix_VolumeMusic(0);
        LOG_DEBUG("SoundManager: Ambience music muted.");
    } else {
        Mix_VolumeMusic(ambienceVolume);
        LOG_DEBUG("SoundManager: Ambience music volume set to " << settings_.interfaceAmbienceVol << "% (SDL_mixer: " << ambienceVolume << ")");
    }

    // Table Music (Mix_Chunk on dedicated channel)
    LOG_DEBUG("SoundManager: Applying table music settings. Mute: " << settings_.tableMusicMute << ", Volume: " << settings_.tableMusicVol);
    int tableVolume = static_cast<int>(settings_.tableMusicVol * MIX_MAX_VOLUME / 100.0f);
    if (settings_.tableMusicMute || !isTableMusicPlaying_) { // Also mute if not playing
        Mix_Volume(tableMusicChannel_, 0); // Mute specific channel
        LOG_DEBUG("SoundManager: Table music muted or not playing on channel " << tableMusicChannel_ << ".");
    } else {
        Mix_Volume(tableMusicChannel_, tableVolume); // Set volume for specific channel
        LOG_DEBUG("SoundManager: Table music volume set to " << settings_.tableMusicVol << "% (SDL_mixer: " << tableVolume << ") on channel " << tableMusicChannel_ << ".");
    }
}

/**
 * @brief Updates the internal settings and reloads sounds if necessary.
 * @param newSettings The new settings to apply.
 */
void SoundManager::updateSettings(const Settings& newSettings) {
    LOG_DEBUG("SoundManager: Updating SoundManager settings.");

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

    // Check if ambience music path or mute settings have changed
    bool ambiencePathChanged = (settings_.ambienceSound != newSettings.ambienceSound);
    bool ambienceMuteChanged = (settings_.interfaceAmbienceMute != newSettings.interfaceAmbienceMute);
    bool ambienceVolumeChanged = (settings_.interfaceAmbienceVol != newSettings.interfaceAmbienceVol);

    // Check if table music mute/volume settings have changed (path change is handled in playTableMusic)
    bool tableMuteChanged = (settings_.tableMusicMute != newSettings.tableMusicMute);
    bool tableVolumeChanged = (settings_.tableMusicVol != newSettings.tableMusicVol);

    // Update the internal settings_ member variable with the new settings
    settings_ = newSettings;

    if (uiSoundPathsChanged) {
        LOG_DEBUG("SoundManager: UI sound paths changed, reloading UI sounds.");
        loadSounds(); // Reload all UI sounds (Mix_Chunk)
    }

    // Handle Ambience Music (always try to keep it playing if configured)
    std::string currentAmbienceFullPath = exeDir_ + settings_.ambienceSound;
    if (!settings_.ambienceSound.empty() && std::filesystem::exists(currentAmbienceFullPath) && std::filesystem::is_regular_file(currentAmbienceFullPath)) {
        if (!isAmbiencePlaying_ || ambiencePathChanged) {
            // Ambience is not playing, or its path changed, so play/re-play it.
            // This will load if necessary and start.
            playAmbienceMusic(currentAmbienceFullPath);
        } else if (ambienceMuteChanged || ambienceVolumeChanged) {
            // Ambience is playing and path hasn't changed, but mute/volume did.
            // Just re-apply settings without restarting.
            applyAudioSettings();
        }
    } else {
        // Ambience path is empty or invalid, ensure it's stopped.
        if (isAmbiencePlaying_) {
            stopAmbienceMusic();
        }
    }

    // Handle Table Music (only need to re-apply settings if volume/mute changed)
    // The path change is implicitly handled when playTableMusic is called by AssetManager upon table change.
    if (isTableMusicPlaying_ && (tableMuteChanged || tableVolumeChanged)) {
        LOG_DEBUG("SoundManager: Table music volume/mute settings changed, reapplying.");
        applyAudioSettings();
    }
    // No else-if here, as ambience takes priority when no table music is active.

    // Always apply audio settings to update volumes/mutes for all categories
    // This call is redundant if individual music types re-apply, but harmless for safety.
    applyAudioSettings();
}

/**
 * @brief Trims leading and trailing whitespace from a string.
 * @param str The string to trim.
 * @return The trimmed string.
 */
std::string SoundManager::trim(const std::string& str) {
    // Find the first non-whitespace character
    size_t first = str.find_first_not_of(" \t\n\r\f\v");
    if (std::string::npos == first) {
        return str; // No non-whitespace characters, return original string (might be empty or all spaces)
    }
    // Find the last non-whitespace character
    size_t last = str.find_last_not_of(" \t\n\r\f\v");
    return str.substr(first, (last - first + 1));
}