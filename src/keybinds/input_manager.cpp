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
        //int result = std::system(command.c_str()); // This call blocks until VPX exits
        int result = std::system((command + " > /dev/null 2>&1").c_str());
        
        inExternalAppMode_ = false; // Reset flag after VPX exits
        lastExternalAppReturnTime_ = SDL_GetTicks(); // Record the time VPX returned for debouncing
        
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

    // Log event type for debugging
    //LOG_DEBUG("InputManager: Processing event type: " << event.type);
    // Process ImGui events first, regardless of other state.
    // ImGui needs to process all events to maintain its internal state correctly.
    ImGui_ImplSDL2_ProcessEvent(&event);

    // Get current time for debounce check
    Uint32 currentTime = SDL_GetTicks();

    // Log ImGui input capture state
    //ImGuiIO& io = ImGui::GetIO();
    //LOG_DEBUG("InputManager: ImGui WantCaptureKeyboard: " << (io.WantCaptureKeyboard ? "true" : "false"));

    // If we are currently in an external application (VPX, screenshot tool, etc.)
    // OR if we are within the debounce period after an external app returned,
    // then ignore all further custom actions to prevent queued inputs from triggering.
    if (inExternalAppMode_ || screenshotManager_->isActive() ||
        (currentTime - lastExternalAppReturnTime_) < EXTERNAL_APP_DEBOUNCE_TIME_MS) {
        //LOG_DEBUG("InputManager: Event ignored due to external app mode or debounce.");
        return; // Skip all other custom input handling
    }

    ImGuiIO& io = ImGui::GetIO();

    // Process specific global keybinds that *should* work even when ImGui has focus,
    // but only if they are not intended as text input.
    if (event.type == SDL_KEYDOWN) {
        // Handle ToggleConfig (C)
        if (keybindProvider_->isAction(event.key, "ToggleConfig")) {
            if (!io.WantCaptureKeyboard) {
                actionHandlers_["ToggleConfig"]();
                return;
            }
        }

        // Handle Quit/ConfigClose (Q) when config is open
        if (*showConfig_ && (keybindProvider_->isAction(event.key, "ConfigClose") || keybindProvider_->isAction(event.key, "Quit"))) {
            if (!io.WantCaptureKeyboard) {
                actionHandlers_["Quit"]();
                return;
            }
        }
    }

    // If ImGui is currently capturing any keyboard input,
    // prevent your custom actions from firing when ImGui has focus.
    if (io.WantCaptureKeyboard) {
        return;
    }

    // Continue with regular event handling for cases where ImGui is not capturing
    // keyboard input and no external app is active/debouncing.
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
            int playfieldX, playfieldY, backglassX, backglassY, dmdX, dmdY;
            windowManager_->getWindowPositions(playfieldX, playfieldY, backglassX, backglassY, dmdX, dmdY);
            settingsManager_->updateWindowPositions(playfieldX, playfieldY, backglassX, backglassY, dmdX, dmdY);
            soundManager_->playUISound("config_save");
            lastClickTimes_.erase(it); // Clear to reset double-click detection
        } else {
            lastClickTimes_[windowID] = currentTime;
        }
    }
}