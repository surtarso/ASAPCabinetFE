#include "keybinds/input_manager.h"
#include "keybinds/keybind_manager.h"
#include "utils/logging.h"
#include "imgui.h"
#include "imgui_impl_sdl2.h" // Required for ImGui_ImplSDL2_ProcessEvent
#include <iostream>
#include <random>
#include <SDL.h> // Ensure SDL.h is included for SDL_GetTicks() and SDL_Event

InputManager::InputManager(IKeybindProvider* keybindProvider)
    : keybindProvider_(keybindProvider), assets_(nullptr), soundManager_(nullptr),
      settingsManager_(nullptr), windowManager_(nullptr), currentIndex_(nullptr),
      tables_(nullptr), showConfig_(nullptr), exeDir_(""), screenshotManager_(nullptr),
      screenshotModeActive_(false),
      inExternalAppMode_(false),
      lastExternalAppReturnTime_(0) // Initialize new timestamp
{}

void InputManager::setDependencies(IAssetManager* assets, ISoundManager* sound, IConfigService* settings,
                                   size_t& currentIndex, const std::vector<TableData>& tables,
                                   bool& showConfig, const std::string& exeDir, IScreenshotManager* screenshotManager,
                                   IWindowManager* windowManager) {
    assets_ = assets;
    soundManager_ = sound;
    settingsManager_ = settings;
    windowManager_ = windowManager;
    currentIndex_ = &currentIndex;
    tables_ = &tables;
    showConfig_ = &showConfig;
    exeDir_ = exeDir;
    screenshotManager_ = screenshotManager;

    for (size_t i = 0; i < tables_->size(); ++i) {
        if (!tables_->at(i).title.empty()) {
            char firstChar = tables_->at(i).title[0];
            char key = std::isalpha(firstChar) ? std::toupper(firstChar) : firstChar;
            if (letterIndex_.find(key) == letterIndex_.end()) {
                letterIndex_[key] = i;
            }
        }
    }
}

void InputManager::registerActions() {
    actionHandlers_["PreviousTable"] = [this]() {
        LOG_DEBUG("InputManager: Previous table triggered");
        size_t newIndex = (*currentIndex_ + tables_->size() - 1) % tables_->size();
        if (newIndex != *currentIndex_) {
            assets_->loadTableAssets(newIndex, *tables_);
            *currentIndex_ = newIndex;
            soundManager_->playUISound("scroll_prev");
        }
    };

    actionHandlers_["NextTable"] = [this]() {
        LOG_DEBUG("InputManager: Next table triggered");
        size_t newIndex = (*currentIndex_ + 1) % tables_->size();
        if (newIndex != *currentIndex_) {
            assets_->loadTableAssets(newIndex, *tables_);
            *currentIndex_ = newIndex;
            soundManager_->playUISound("scroll_next");
        }
    };

    actionHandlers_["FastPrevTable"] = [this]() {
        LOG_DEBUG("InputManager: Fast previous table triggered");
        size_t newIndex = (*currentIndex_ + tables_->size() - 10) % tables_->size();
        if (newIndex != *currentIndex_) {
            assets_->loadTableAssets(newIndex, *tables_);
            *currentIndex_ = newIndex;
            soundManager_->playUISound("scroll_fast_prev");
        }
    };

    actionHandlers_["FastNextTable"] = [this]() {
        LOG_DEBUG("InputManager: Fast next table triggered");
        size_t newIndex = (*currentIndex_ + 10) % tables_->size();
        if (newIndex != *currentIndex_) {
            assets_->loadTableAssets(newIndex, *tables_);
            *currentIndex_ = newIndex;
            soundManager_->playUISound("scroll_fast_next");
        }
    };

    actionHandlers_["JumpPrevLetter"] = [this]() {
        LOG_DEBUG("InputManager: Jump previous letter triggered");
        char currentChar = tables_->at(*currentIndex_).title[0];
        char key = std::isalpha(currentChar) ? std::toupper(currentChar) : currentChar;
        auto it = letterIndex_.find(key);
        if (it != letterIndex_.begin()) {
            auto prevIt = std::prev(it);
            size_t newIndex = prevIt->second;
            if (newIndex != *currentIndex_) {
                assets_->loadTableAssets(newIndex, *tables_);
                *currentIndex_ = newIndex;
                soundManager_->playUISound("scroll_jump_prev");
            }
        } else {
            auto lastIt = std::prev(letterIndex_.end());
            size_t newIndex = lastIt->second;
            if (newIndex != *currentIndex_) {
                assets_->loadTableAssets(newIndex, *tables_);
                *currentIndex_ = newIndex;
                soundManager_->playUISound("scroll_jump_prev");
            }
        }
    };

    actionHandlers_["JumpNextLetter"] = [this]() {
        LOG_DEBUG("InputManager: Jump next letter triggered");
        char currentChar = tables_->at(*currentIndex_).title[0];
        char key = std::isalpha(currentChar) ? std::toupper(currentChar) : currentChar;
        auto it = letterIndex_.find(key);
        if (it != letterIndex_.end() && std::next(it) != letterIndex_.end()) {
            size_t newIndex = std::next(it)->second;
            if (newIndex != *currentIndex_) {
                assets_->loadTableAssets(newIndex, *tables_);
                *currentIndex_ = newIndex;
                soundManager_->playUISound("scroll_jump_next");
            }
        } else {
            size_t newIndex = letterIndex_.begin()->second;
            if (newIndex != *currentIndex_) {
                assets_->loadTableAssets(newIndex, *tables_);
                *currentIndex_ = newIndex;
                soundManager_->playUISound("scroll_jump_next");
            }
        }
    };

    actionHandlers_["RandomTable"] = [this]() {
        LOG_DEBUG("InputManager: Random table triggered");
        if (!tables_->empty()) {
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<size_t> dist(0, tables_->size() - 1);
            size_t newIndex = dist(gen);
            if (newIndex != *currentIndex_) {
                assets_->loadTableAssets(newIndex, *tables_);
                *currentIndex_ = newIndex;
                soundManager_->playUISound("scroll_random");
            }
        }
    };

    actionHandlers_["LaunchTable"] = [this]() {
        // Prevent launch if already in an external app mode (VPX is running)
        // or if within the debounce period after an external app returned.
        if (inExternalAppMode_) {
            LOG_DEBUG("InputManager: Launch skipped, already in external app mode.");
            return;
        }

        Uint32 currentTime = SDL_GetTicks();
        if ((currentTime - lastExternalAppReturnTime_) < EXTERNAL_APP_DEBOUNCE_TIME_MS) {
            LOG_DEBUG("InputManager: Launch skipped, debouncing after external app return.");
            return;
        }

        inExternalAppMode_ = true; // Set flag to indicate external app is launching
        LOG_DEBUG("InputManager: Launch table triggered");
        soundManager_->playUISound("launch_table");

        const Settings& settings = settingsManager_->getSettings();
        std::string command = settings.vpxStartArgs + " " + settings.VPinballXPath + " " +
                            settings.vpxSubCmd + " \"" + tables_->at(*currentIndex_).vpxFile + "\" " +
                            settings.vpxEndArgs;

        LOG_DEBUG("InputManager: Launching: " << command);

        //stop ambience/table music
        soundManager_->stopMusic();
        LOG_DEBUG("All music stopped.");
        //stop video players
        if (IVideoPlayer* player = assets_->getPlayfieldVideoPlayer()) {
            player->stop();
            LOG_DEBUG("InputManager: Stopped playfield video player");
        }
        if (IVideoPlayer* player = assets_->getBackglassVideoPlayer()) {
            player->stop();
            LOG_DEBUG("InputManager: Stopped backglass video player");
        }
        if (IVideoPlayer* player = assets_->getDmdVideoPlayer()) {
            player->stop();
            LOG_DEBUG("InputManager: Stopped DMD video player");
        }
        //keep ambience on background while playing
        //TODO: this should be optional.
        //soundManager_->playAmbienceMusic(settings.ambienceSound);

        //int result = std::system(command.c_str()); // This call blocks until VPX exits
        int result = std::system((command + " > /dev/null 2>&1").c_str());
        
        inExternalAppMode_ = false; // Reset flag after VPX exits
        lastExternalAppReturnTime_ = SDL_GetTicks(); // Record the time VPX returned for debouncing
        
        //play table music on return, fallback to ambience
        soundManager_->playTableMusic(tables_->at(*currentIndex_).music);
        LOG_DEBUG("Music resumed.");
        //resume video players
        if (IVideoPlayer* player = assets_->getPlayfieldVideoPlayer()) {
            player->play();
            LOG_DEBUG("InputManager: Resumed playfield video player");
        }
        if (IVideoPlayer* player = assets_->getBackglassVideoPlayer()) {
            player->play();
            LOG_DEBUG("InputManager: Resumed backglass video player");
        }
        if (IVideoPlayer* player = assets_->getDmdVideoPlayer()) {
            player->play();
            LOG_DEBUG("InputManager: Resumed DMD video player");
        }
        if (result != 0) {
            LOG_ERROR("InputManager: Warning: VPX launch failed with exit code " << result);
        }
    };


    actionHandlers_["ScreenshotMode"] = [this]() {
        // Prevent launching screenshot mode while in another external app
        // or if within the debounce period after an external app returned.
        if (inExternalAppMode_) {
            LOG_DEBUG("InputManager: Screenshot mode skipped, already in external app mode.");
            return;
        }

        Uint32 currentTime = SDL_GetTicks();
        if ((currentTime - lastExternalAppReturnTime_) < EXTERNAL_APP_DEBOUNCE_TIME_MS) {
            LOG_DEBUG("InputManager: Screenshot mode skipped, debouncing after external app return.");
            return;
        }

        LOG_DEBUG("InputManager: Screenshot mode triggered");
        if (!screenshotModeActive_) { // Use the specific flag for the action's internal state
            soundManager_->playUISound("launch_screenshot");
            screenshotModeActive_ = true; // Set internal flag
            inExternalAppMode_ = true; // Also set general external app flag
            
            screenshotManager_->launchScreenshotMode(tables_->at(*currentIndex_).vpxFile);
            
            inExternalAppMode_ = false; // Reset general external app flag
            screenshotModeActive_ = false; // Reset internal flag
            lastExternalAppReturnTime_ = SDL_GetTicks(); // Record return time
            LOG_DEBUG("InputManager: Exited screenshot mode");
        }
    };

    actionHandlers_["ToggleConfig"] = [this]() {
        LOG_DEBUG("InputManager: ToggleConfig action triggered");
        soundManager_->playUISound("config_toggle");
        *showConfig_ = !*showConfig_;
        LOG_DEBUG("InputManager: Toggled showConfig to: " << (*showConfig_ ? 1 : 0));
    };

    actionHandlers_["Quit"] = [this]() {
        LOG_DEBUG("InputManager: Quit triggered");
        if (screenshotModeActive_) { // Check specific screenshot mode flag
            screenshotModeActive_ = false;
            LOG_DEBUG("InputManager: Exited screenshot mode (quit skipped)");
        } else if (*showConfig_) {
            *showConfig_ = false;
            LOG_DEBUG("InputManager: Closed config");
        } else {
            quit_ = true;
            LOG_DEBUG("InputManager: Quitting app");
        }
    };

    actionHandlers_["ConfigClose"] = [this]() {
        LOG_DEBUG("InputManager: ConfigClose triggered");
        *showConfig_ = false;
    };
}

void InputManager::handleEvent(const SDL_Event& event) {
    if (event.type == SDL_QUIT) {
        quit_ = true;
        LOG_DEBUG("InputManager: SDL_QUIT received");
        return;
    }

    if (event.type == SDL_JOYDEVICEADDED || event.type == SDL_JOYDEVICEREMOVED) {
        return; 
    }

    // Process ImGui events first, always.
    ImGui_ImplSDL2_ProcessEvent(&event);

    Uint32 currentTime = SDL_GetTicks();
    ImGuiIO& io = ImGui::GetIO(); // Get ImGuiIO state here once

    // Priority 1: External application active or debounce period
    // If an external app is running or recently returned, block all custom input.
    if (inExternalAppMode_ || screenshotManager_->isActive() ||
        (currentTime - lastExternalAppReturnTime_) < EXTERNAL_APP_DEBOUNCE_TIME_MS) {
        return; 
    }

    // Priority 2: Handle 'ToggleConfig' (e.g., 'C' key)
    // This action should always work to open or close the config UI,
    // UNLESS ImGui is actively capturing keyboard input (e.g., in a text field).
    if (event.type == SDL_KEYDOWN && keybindProvider_->isAction(event.key, "ToggleConfig")) {
        if (!io.WantCaptureKeyboard) { // If user is not typing in an ImGui text field
            actionHandlers_["ToggleConfig"](); // This will toggle *showConfig_
            return; // Consume the event, no further processing needed
        }
    }

    // Priority 3: Config UI is open (*showConfig_ == true)
    // If the config UI is currently open, we should only allow specific config-related actions.
    // All other game actions (like ScreenshotMode) should be blocked.
    if (*showConfig_) {
        if (event.type == SDL_KEYDOWN) {
            // Allow 'ConfigClose' or 'Quit' action (e.g., 'Q' key) to close the config or quit the app.
            if (!io.WantCaptureKeyboard) { // Ensure user isn't typing in a config text field
                if (keybindProvider_->isAction(event.key, "ConfigClose") || keybindProvider_->isAction(event.key, "Quit")) {
                    actionHandlers_["Quit"](); // The "Quit" handler manages closing the config or truly quitting
                    return; // Consume the event
                }
            }
        }
        // If config is open and the event wasn't handled by 'ToggleConfig' or 'ConfigClose/Quit',
        // then block all other custom actions (including ScreenshotMode).
        return; 
    }

    // Priority 4: ImGui is capturing keyboard input (e.g., a text field in the main UI)
    // This check applies if the config is *not* open, but another ImGui element has focus.
    if (io.WantCaptureKeyboard) {
        return; // Block custom game actions, let ImGui handle the key press.
    }

    // Priority 5: Regular game event handling
    // If none of the above conditions are met (no external app, config is closed,
    // and ImGui is not capturing keyboard for text input), then process regular game actions.
    handleRegularEvents(event);
    handleDoubleClick(event);
}

void InputManager::handleRegularEvents(const SDL_Event& event) {
    for (const auto& action : keybindProvider_->getActions()) {
        // Skip config and screenshot-specific actions as they are handled globally or specifically
        if (action == "ConfigClose" || action == "ScreenshotKey" || action == "ScreenshotQuit" || action == "ToggleConfig") {
            continue; 
        }

        if (event.type == SDL_KEYDOWN && keybindProvider_->isAction(event.key, action)) {
            auto it = actionHandlers_.find(action);
            if (it != actionHandlers_.end()) {
                it->second();
                // Ensure only one action per keydown event to prevent unwanted cascading
                return; 
            }
        } else if (event.type == SDL_JOYBUTTONDOWN && keybindProvider_->isJoystickAction(event.jbutton, action)) {
            auto it = actionHandlers_.find(action);
            if (it != actionHandlers_.end()) {
                it->second();
                return;
            }
        } else if (event.type == SDL_JOYHATMOTION && keybindProvider_->isJoystickHatAction(event.jhat, action)) {
            auto it = actionHandlers_.find(action);
            if (it != actionHandlers_.end()) {
                it->second();
                return;
            }
        } else if (event.type == SDL_JOYAXISMOTION && keybindProvider_->isJoystickAxisAction(event.jaxis, action)) {
            auto it = actionHandlers_.find(action);
            if (it != actionHandlers_.end()) {
                it->second();
                return;
            }
        }
    }
}

void InputManager::handleDoubleClick(const SDL_Event& event) {
    static const Uint32 DOUBLE_CLICK_TIME = 300;
    if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
        Uint32 windowID = event.button.windowID;
        Uint32 currentTime = SDL_GetTicks();
        auto it = lastClickTimes_.find(windowID);
        if (it != lastClickTimes_.end() && (currentTime - it->second) <= DOUBLE_CLICK_TIME) {
            LOG_DEBUG("InputManager: Double-click detected on window ID: " << windowID);
            int playfieldX, playfieldY, backglassX, backglassY, dmdX, dmdY, topperX, topperY;
            windowManager_->getWindowPositions(playfieldX, playfieldY, backglassX, backglassY, dmdX, dmdY, topperX, topperY);
            settingsManager_->updateWindowPositions(playfieldX, playfieldY, backglassX, backglassY, dmdX, dmdY, topperX, topperY);
            soundManager_->playUISound("config_save");
            lastClickTimes_.erase(it); // Clear to reset double-click detection
        } else {
            lastClickTimes_[windowID] = currentTime;
        }
    }
}