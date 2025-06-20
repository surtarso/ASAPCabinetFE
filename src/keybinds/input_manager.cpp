#include "keybinds/input_manager.h"
#include "log/logging.h"
#include "imgui.h"
#include "backends/imgui_impl_sdl2.h" // Required for ImGui_ImplSDL2_ProcessEvent
#include <iostream>
#include <random>
#include <SDL.h> // For SDL_GetTicks() and SDL_Event
#include <sstream>
#include <chrono> // for playTime
#include <string> // for time format

InputManager::InputManager(IKeybindProvider* keybindProvider)
    : keybindProvider_(keybindProvider), assets_(nullptr), soundManager_(nullptr),
      settingsManager_(nullptr), windowManager_(nullptr), currentIndex_(nullptr),
      tables_(nullptr), showConfig_(nullptr), showEditor_(nullptr), showVpsdb_(nullptr), exeDir_(""), screenshotManager_(nullptr),
      runtimeEditor_(nullptr), actionHandlers_(), letterIndex_(), quit_(false),
      screenshotModeActive_(false), lastClickTimes_(), inExternalAppMode_(false),
      lastExternalAppReturnTime_(0), tableLauncher_(nullptr) {
    registerActions();
    //LOG_INFO("InputManager: Constructor started, quit_ = " << quit_);
}

void InputManager::setDependencies(IAssetManager* assets, ISoundManager* sound, IConfigService* settings,
                                   size_t& currentIndex, const std::vector<TableData>& tables,
                                   bool& showConfig, bool& showEditor, bool& showVpsdb, const std::string& exeDir, IScreenshotManager* screenshotManager,
                                   IWindowManager* windowManager, std::atomic<bool>& isLoadingTables, ITableLauncher* tableLauncher) {
    //LOG_INFO("InputManager: setDependencies started, quit_ = " << quit_);
    assets_ = assets;
    soundManager_ = sound;
    settingsManager_ = settings;
    windowManager_ = windowManager;
    currentIndex_ = &currentIndex;
    tables_ = &tables;
    showConfig_ = &showConfig;
    showEditor_ = &showEditor;
    showVpsdb_ = &showVpsdb;
    exeDir_ = exeDir;
    screenshotManager_ = screenshotManager;
    isLoadingTables_ = &isLoadingTables;
    tableLauncher_ = tableLauncher;

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
    actionHandlers_["Previous Table"] = [this]() {
        LOG_DEBUG("InputManager: Previous table triggered");
        size_t newIndex = (*currentIndex_ + tables_->size() - 1) % tables_->size();
        if (newIndex != *currentIndex_) {
            assets_->loadTableAssets(newIndex, *tables_);
            *currentIndex_ = newIndex;
            soundManager_->playUISound("scroll_normal");
        }
        LOG_DEBUG("InputManager: Registered " << actionHandlers_.size() << " action handlers");
    };

    actionHandlers_["Next Table"] = [this]() {
        LOG_DEBUG("InputManager: Next table triggered");
        size_t newIndex = (*currentIndex_ + 1) % tables_->size();
        if (newIndex != *currentIndex_) {
            assets_->loadTableAssets(newIndex, *tables_);
            *currentIndex_ = newIndex;
            soundManager_->playUISound("scroll_normal");
        }
    };

    actionHandlers_["Fast Previous Table"] = [this]() {
        LOG_DEBUG("InputManager: Fast previous table triggered");
        size_t newIndex = (*currentIndex_ + tables_->size() - 10) % tables_->size();
        if (newIndex != *currentIndex_) {
            assets_->loadTableAssets(newIndex, *tables_);
            *currentIndex_ = newIndex;
            soundManager_->playUISound("scroll_fast");
        }
    };

    actionHandlers_["Fast Next Table"] = [this]() {
        LOG_DEBUG("InputManager: Fast next table triggered");
        size_t newIndex = (*currentIndex_ + 10) % tables_->size();
        if (newIndex != *currentIndex_) {
            assets_->loadTableAssets(newIndex, *tables_);
            *currentIndex_ = newIndex;
            soundManager_->playUISound("scroll_fast");
        }
    };

    actionHandlers_["Jump Previous Letter"] = [this]() {
        LOG_DEBUG("InputManager: Jump previous letter triggered");
        if (!tables_ || tables_->empty() || !currentIndex_) {
            LOG_ERROR("InputManager: Invalid tables or currentIndex for JumpPrevLetter");
            return;
        }

        size_t currentIdx = *currentIndex_;
        if (currentIdx >= tables_->size()) {
            LOG_ERROR("InputManager: currentIndex " << currentIdx << " out of range (size=" << tables_->size() << ")");
            return;
        }

        std::string currentTitle = tables_->at(currentIdx).title;
        if (currentTitle.empty()) {
            LOG_ERROR("InputManager: Empty title at index " << currentIdx);
            return;
        }

        char currentChar = currentTitle[0];
        char currentKey = (std::isalpha(currentChar) || std::isdigit(currentChar)) ? std::toupper(currentChar) : '\0';
        if (currentKey == '\0') {
            LOG_ERROR("InputManager: Invalid first character in title: " << currentTitle);
            return;
        }

        size_t newIndex = currentIdx;
        bool found = false;

        // Scan backward for a different letter
        for (size_t i = currentIdx; i > 0; --i) {
            size_t idx = i - 1;
            std::string title = tables_->at(idx).title;
            if (title.empty()) continue;
            char c = title[0];
            char key = (std::isalpha(c) || std::isdigit(c)) ? std::toupper(c) : '\0';
            if (key != '\0' && key < currentKey) {
                newIndex = idx;
                found = true;
                break;
            }
        }

        // Wrap to the highest letter if no previous letter found
        if (!found) {
            for (size_t i = tables_->size(); i > 0; --i) {
                size_t idx = i - 1;
                std::string title = tables_->at(idx).title;
                if (title.empty()) continue;
                char c = title[0];
                char key = (std::isalpha(c) || std::isdigit(c)) ? std::toupper(c) : '\0';
                if (key != '\0') {
                    newIndex = idx;
                    found = true;
                    break;
                }
            }
        }

        if (found && newIndex != currentIdx) {
            assets_->loadTableAssets(newIndex, *tables_);
            *currentIndex_ = newIndex;
            soundManager_->playUISound("scroll_jump");
        } else {
            LOG_DEBUG("InputManager: No previous letter found for key " << currentKey);
        }
    };

    actionHandlers_["Jump Next Letter"] = [this]() {
        LOG_DEBUG("InputManager: Jump next letter triggered");
        if (!tables_ || tables_->empty() || !currentIndex_) {
            LOG_ERROR("InputManager: Invalid tables or currentIndex for JumpNextLetter");
            return;
        }

        size_t currentIdx = *currentIndex_;
        if (currentIdx >= tables_->size()) {
            LOG_ERROR("InputManager: currentIndex " << currentIdx << " out of range (size=" << tables_->size() << ")");
            return;
        }

        std::string currentTitle = tables_->at(currentIdx).title;
        if (currentTitle.empty()) {
            LOG_ERROR("InputManager: Empty title at index " << currentIdx);
            return;
        }

        char currentChar = currentTitle[0];
        char currentKey = (std::isalpha(currentChar) || std::isdigit(currentChar)) ? std::toupper(currentChar) : '\0';
        if (currentKey == '\0') {
            LOG_ERROR("InputManager: Invalid first character in title: " << currentTitle);
            return;
        }

        size_t newIndex = currentIdx;
        bool found = false;

        // Scan forward for a different letter
        for (size_t i = currentIdx + 1; i < tables_->size(); ++i) {
            std::string title = tables_->at(i).title;
            if (title.empty()) continue;
            char c = title[0];
            char key = (std::isalpha(c) || std::isdigit(c)) ? std::toupper(c) : '\0';
            if (key != '\0' && key > currentKey) {
                newIndex = i;
                found = true;
                break;
            }
        }

        // Wrap to the lowest letter if no next letter found
        if (!found) {
            for (size_t i = 0; i < tables_->size(); ++i) {
                std::string title = tables_->at(i).title;
                if (title.empty()) continue;
                char c = title[0];
                char key = (std::isalpha(c) || std::isdigit(c)) ? std::toupper(c) : '\0';
                if (key != '\0') {
                    newIndex = i;
                    found = true;
                    break;
                }
            }
        }

        if (found && newIndex != currentIdx) {
            assets_->loadTableAssets(newIndex, *tables_);
            *currentIndex_ = newIndex;
            soundManager_->playUISound("scroll_jump");
        } else {
            LOG_DEBUG("InputManager: No next letter found for key " << currentKey);
        }
    };

    actionHandlers_["Random Table"] = [this]() {
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

    actionHandlers_["Launch Table"] = [this]() {
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

        if (!tableLauncher_) {
            LOG_ERROR("InputManager: Cannot launch table, tableLauncher_ is null");
            return;
        }

        inExternalAppMode_ = true; // Set flag to indicate external app is launching
        LOG_DEBUG("InputManager: Launch table triggered");

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
        if (IVideoPlayer* player = assets_->getTopperVideoPlayer()) {
            player->stop();
            LOG_DEBUG("InputManager: Stopped topper video player");
        }
        // Play launch sound
        if (tables_->at(*currentIndex_).launchAudio == "") {
            soundManager_->playUISound("launch_table");
        } else {
            soundManager_->playCustomLaunch(tables_->at(*currentIndex_).launchAudio);
        }

        // Launch table
        auto [result, timePlayed] = tableLauncher_->launchTable(tables_->at(*currentIndex_));

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
        if (IVideoPlayer* player = assets_->getTopperVideoPlayer()) {
            player->play();
            LOG_DEBUG("InputManager: Resumed topper video player");
        }
        if (result != 0) {
            LOG_ERROR("InputManager: Warning: VPX launch failed with exit code " << result);
        }

        inExternalAppMode_ = false; // Reset flag after VPX exits
        lastExternalAppReturnTime_ = SDL_GetTicks(); // Record the time VPX returned for debouncing
        
    };


    actionHandlers_["Screenshot Mode"] = [this]() {
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

    actionHandlers_["Toggle Config"] = [this]() {
        LOG_DEBUG("InputManager: ToggleConfig action triggered");
        soundManager_->playUISound("panel_toggle");
        *showConfig_ = !*showConfig_;
        LOG_DEBUG("InputManager: Toggled showConfig to: " << (*showConfig_ ? 1 : 0));
    };

    actionHandlers_["Toggle Editor"] = [this]() {
        LOG_DEBUG("InputManager: MetadataEdit action triggered");
        soundManager_->playUISound("panel_toggle");
        *showEditor_ = !*showEditor_;
        LOG_DEBUG("InputManager: Toggled showEditor to: " << (*showEditor_ ? 1 : 0));
    };

    actionHandlers_["Toggle Catalog"] = [this]() {
        LOG_DEBUG("InputManager: MetadataCatalog action triggered");
        soundManager_->playUISound("panel_toggle");
        *showVpsdb_ = !*showVpsdb_;
        LOG_DEBUG("InputManager: Toggled showVpsdb to: " << (*showVpsdb_ ? 1 : 0));
    };

    actionHandlers_["Quit"] = [this]() {
        LOG_DEBUG("InputManager: Quit triggered");
        if (screenshotModeActive_) { // Check specific screenshot mode flag
            screenshotModeActive_ = false;
            LOG_DEBUG("InputManager: Exited screenshot mode (quit skipped)");
        } else if (*showConfig_) {
            *showConfig_ = false;
            LOG_DEBUG("InputManager: Closed Config UI");
        } else if (*showEditor_) {
            *showEditor_ = false;
            LOG_DEBUG("InputManager: Closed Editor");
        } else if (*showVpsdb_) {
            *showVpsdb_ = false;
            LOG_DEBUG("InputManager: Closed Catalog");
        } else {
            quit_ = true;
            LOG_DEBUG("InputManager: Quitting app");
        }
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

    // Block all input if tables are loading, except for quit
    if (isLoadingTables_ && *isLoadingTables_) {
        if (event.type == SDL_KEYDOWN && keybindProvider_->isAction(event.key, "Quit")) {
            actionHandlers_["Quit"]();
        }
        return; // Block all other actions
    }

    // Priority 1: External application active or debounce period
    // If an external app is running or recently returned, block all custom input.
    if (inExternalAppMode_ || screenshotManager_->isActive() ||
        (currentTime - lastExternalAppReturnTime_) < EXTERNAL_APP_DEBOUNCE_TIME_MS) {
        return; 
    }

    // Priority 2: Handle 'ToggleConfig' (e.g., 'C' key)
    // This action should always work to open or close the config UI,
    // UNLESS ImGui is actively capturing keyboard input (e.g., in a text field).
    if (event.type == SDL_KEYDOWN && !io.WantCaptureKeyboard) {
        if (keybindProvider_->isAction(event.key, "Toggle Config")) {
            actionHandlers_["Toggle Config"](); // Toggle showConfig_ with sound
            return; // Consume the event
        }
        if (keybindProvider_->isAction(event.key, "Toggle Editor")) {
            actionHandlers_["Toggle Editor"](); // Toggle showEditor_ with sound
            return; // Consume the event
        }
        if (keybindProvider_->isAction(event.key, "Toggle Catalog")) {
            actionHandlers_["Toggle Catalog"](); // Toggle showVpsdb_ with sound
            return; // Consume the event
        }
    }

    // Priority 3: Panel open (ex: *showConfig_ == true)
    // If a panel is currently open, we should only allow specific actions.
    // All other game actions (like ScreenshotMode) should be blocked.
    if (*showConfig_) {
        if (event.type == SDL_KEYDOWN) {
            // Allow 'ConfigClose' or 'Quit' action (e.g., 'Q' key) to close the config or quit the app.
            if (!io.WantCaptureKeyboard) { // Ensure user isn't typing in a config text field
                if (keybindProvider_->isAction(event.key, "Toggle Config") || keybindProvider_->isAction(event.key, "Quit")) {
                    actionHandlers_["Quit"](); // The "Quit" handler manages closing the config or truly quitting
                    return; // Consume the event
                }
            }
        }
        // If config is open and the event wasn't handled by 'ToggleConfig' or 'Quit',
        // then block all other custom actions (including ScreenshotMode).
        return; 
    }

    if (*showEditor_) {
        if (event.type == SDL_KEYDOWN) {
            // Allow 'Toggle Editor' or 'Quit' action (e.g., 'Q' key) to close the Toggle Editor or quit the app.
            if (!io.WantCaptureKeyboard) { // Ensure user isn't typing in a Toggle Editor text field
                if (keybindProvider_->isAction(event.key, "Toggle Editor") || keybindProvider_->isAction(event.key, "Quit")) {
                    actionHandlers_["Quit"](); // The "Quit" handler manages closing the Toggle Editor or truly quitting
                    return; // Consume the event
                }
                if (keybindProvider_->isAction(event.key, "Previous Table")) {
                    actionHandlers_["Previous Table"](); // Handle navigation to previous table
                    return; // Consume the event
                }
                if (keybindProvider_->isAction(event.key, "Next Table")) {
                    actionHandlers_["Next Table"](); // Handle navigation to next table
                    return; // Consume the event
                }
            }
        }
        // If Toggle Editor is open and the event wasn't handled by 'Toggle Editor' or 'Quit',
        // then block all other custom actions (including ScreenshotMode).
        return; 
    }

    if (*showVpsdb_) {
        if (event.type == SDL_KEYDOWN) {
            // Allow 'Toggle Catalog' or 'Quit' action (e.g., 'Q' key) to close the Toggle Catalog or quit the app.
            if (!io.WantCaptureKeyboard) { // Ensure user isn't typing in a Toggle Catalog text field
                if (keybindProvider_->isAction(event.key, "Toggle Catalog") || keybindProvider_->isAction(event.key, "Quit")) {
                    actionHandlers_["Quit"](); // The "Quit" handler manages closing the Toggle Catalog or truly quitting
                    return; // Consume the event
                }
            }
        }
        // If Toggle Catalog is open and the event wasn't handled by 'Toggle Catalog' or 'Quit',
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
        if (action == "Screenshot Key" || action == "Screenshot Quit") {
            continue;
        }
        if (event.type == SDL_KEYDOWN && keybindProvider_->isAction(event.key, action)) {
            auto it = actionHandlers_.find(action);
            if (it != actionHandlers_.end()) {
                LOG_DEBUG("InputManager: Triggering action: " << action);
                it->second();
                return;
            } else {
                LOG_DEBUG("InputManager: No handler for action: " << action);
            }
        } else if (event.type == SDL_JOYBUTTONDOWN && keybindProvider_->isJoystickAction(event.jbutton, action)) {
            auto it = actionHandlers_.find(action);
            if (it != actionHandlers_.end()) {
                LOG_DEBUG("InputManager: Triggering joystick action: " << action);
                it->second();
                return;
            }
        } else if (event.type == SDL_JOYHATMOTION && keybindProvider_->isJoystickHatAction(event.jhat, action)) {
            auto it = actionHandlers_.find(action);
            if (it != actionHandlers_.end()) {
                LOG_DEBUG("InputManager: Triggering joystick hat action: " << action);
                it->second();
                return;
            }
        } else if (event.type == SDL_JOYAXISMOTION && keybindProvider_->isJoystickAxisAction(event.jaxis, action)) {
            auto it = actionHandlers_.find(action);
            if (it != actionHandlers_.end()) {
                LOG_DEBUG("InputManager: Triggering joystick axis action: " << action);
                it->second();
                return;
            }
        }
    }
    //LOG_DEBUG("InputManager: No action triggered for event type: " << event.type);
}

void InputManager::handleDoubleClick(const SDL_Event& event) {
    static const Uint32 DOUBLE_CLICK_TIME = 300;
    if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
        Uint32 windowID = event.button.windowID;
        Uint32 currentTime = SDL_GetTicks();
        auto it = lastClickTimes_.find(windowID);
        if (it != lastClickTimes_.end() && (currentTime - it->second) <= DOUBLE_CLICK_TIME) {
            LOG_DEBUG("InputManager: Double-click detected on window ID: " << windowID);

            int playfieldX, playfieldY, playfieldWidth, playfieldHeight,
                backglassX, backglassY, backglassWidth, backglassHeight,
                dmdX, dmdY, dmdWidth, dmdHeight,
                topperX, topperY, topperWidth, topperHeight;

            windowManager_->getWindowSetup(playfieldX, playfieldY, playfieldWidth, playfieldHeight,
                                            backglassX, backglassY, backglassWidth, backglassHeight,
                                            dmdX, dmdY, dmdWidth, dmdHeight,
                                            topperX, topperY, topperWidth, topperHeight);

            settingsManager_->updateWindowSetup(playfieldX, playfieldY, playfieldWidth, playfieldHeight,
                                            backglassX, backglassY, backglassWidth, backglassHeight,
                                            dmdX, dmdY, dmdWidth, dmdHeight,
                                            topperX, topperY, topperWidth, topperHeight);

            soundManager_->playUISound("screenshot_take");
            lastClickTimes_.erase(it); // Clear to reset double-click detection
        } else {
            lastClickTimes_[windowID] = currentTime;
        }
    }
}