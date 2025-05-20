#include "keybinds/input_manager.h"
#include "keybinds/keybind_manager.h"
#include "utils/logging.h"
#include "imgui.h"
#include <iostream>
#include <random>

InputManager::InputManager(IKeybindProvider* keybindProvider)
    : keybindProvider_(keybindProvider), assets_(nullptr), soundManager_(nullptr),
      settingsManager_(nullptr), windowManager_(nullptr), currentIndex_(nullptr),
      tables_(nullptr), showConfig_(nullptr), exeDir_(""), screenshotManager_(nullptr) {}

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
            soundManager_->playSound("scroll_prev");
        }
    };

    actionHandlers_["NextTable"] = [this]() {
        LOG_DEBUG("InputManager: Next table triggered");
        size_t newIndex = (*currentIndex_ + 1) % tables_->size();
        if (newIndex != *currentIndex_) {
            assets_->loadTableAssets(newIndex, *tables_);
            *currentIndex_ = newIndex;
            soundManager_->playSound("scroll_next");
        }
    };

    actionHandlers_["FastPrevTable"] = [this]() {
        LOG_DEBUG("InputManager: Fast previous table triggered");
        size_t newIndex = (*currentIndex_ + tables_->size() - 10) % tables_->size();
        if (newIndex != *currentIndex_) {
            assets_->loadTableAssets(newIndex, *tables_);
            *currentIndex_ = newIndex;
            soundManager_->playSound("scroll_fast_prev");
        }
    };

    actionHandlers_["FastNextTable"] = [this]() {
        LOG_DEBUG("InputManager: Fast next table triggered");
        size_t newIndex = (*currentIndex_ + 10) % tables_->size();
        if (newIndex != *currentIndex_) {
            assets_->loadTableAssets(newIndex, *tables_);
            *currentIndex_ = newIndex;
            soundManager_->playSound("scroll_fast_next");
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
                soundManager_->playSound("scroll_jump_prev");
            }
        } else {
            auto lastIt = std::prev(letterIndex_.end());
            size_t newIndex = lastIt->second;
            if (newIndex != *currentIndex_) {
                assets_->loadTableAssets(newIndex, *tables_);
                *currentIndex_ = newIndex;
                soundManager_->playSound("scroll_jump_prev");
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
                soundManager_->playSound("scroll_jump_next");
            }
        } else {
            size_t newIndex = letterIndex_.begin()->second;
            if (newIndex != *currentIndex_) {
                assets_->loadTableAssets(newIndex, *tables_);
                *currentIndex_ = newIndex;
                soundManager_->playSound("scroll_jump_next");
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
                soundManager_->playSound("scroll_random");
            }
        }
    };

    actionHandlers_["LaunchTable"] = [this]() {
        LOG_DEBUG("InputManager: Launch table triggered");
        soundManager_->playSound("launch_table");
        const Settings& settings = settingsManager_->getSettings();
        std::string command = settings.vpxStartArgs + " " + settings.VPinballXPath + " " +
                              settings.vpxSubCmd + " \"" + tables_->at(*currentIndex_).vpxFile + "\" " +
                              settings.vpxEndArgs;
        LOG_DEBUG("InputManager: Launching: " << command);
        int result = std::system(command.c_str());
        if (result != 0) {
            LOG_ERROR("InputManager: Warning: VPX launch failed with exit code " << result);
        }
    };

    actionHandlers_["ScreenshotMode"] = [this]() {
        LOG_DEBUG("InputManager: Screenshot mode triggered");
        if (!inScreenshotMode_) {
            soundManager_->playSound("launch_screenshot");
            inScreenshotMode_ = true;
            screenshotManager_->launchScreenshotMode(tables_->at(*currentIndex_).vpxFile);
            inScreenshotMode_ = false;
            LOG_DEBUG("InputManager: Exited screenshot mode");
        }
    };

    actionHandlers_["ToggleConfig"] = [this]() {
        LOG_DEBUG("InputManager: ToggleConfig action triggered");
        soundManager_->playSound("config_toggle");
        *showConfig_ = !*showConfig_;
        LOG_DEBUG("InputManager: Toggled showConfig to: " << (*showConfig_ ? 1 : 0));
    };

    actionHandlers_["Quit"] = [this]() {
        LOG_DEBUG("InputManager: Quit triggered");
        if (inScreenshotMode_) {
            inScreenshotMode_ = false;
            LOG_DEBUG("InputManager: Exited screenshot mode (quit skipped)");
        } else if (*showConfig_) {
            *showConfig_ = false;
            LOG_DEBUG("InputManager: Closed config");
        } else {
            quit_ = true;
            LOG_INFO("InputManager: Quitting app");
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
        return; // Handled in App
    }

    if (screenshotManager_->isActive()) {
        return; // Let ScreenshotManager handle its own events
    }

    ImGuiIO& io = ImGui::GetIO();

    // Process specific global keybinds that *should* work even when ImGui has focus,
    // but only if they are not intended as text input.
    // For 'C' (ToggleConfig) and 'Q' (Quit/ConfigClose), if ImGui is capturing keyboard,
    // it implies the key is for text input, so we should bypass the global action.
    if (event.type == SDL_KEYDOWN) {
        // Handle ToggleConfig (C)
        if (keybindProvider_->isAction(event.key, "ToggleConfig")) {
            // Only activate global toggle if ImGui is NOT currently capturing keyboard input.
            // This allows 'C' to be typed into a text field when focused.
            if (!io.WantCaptureKeyboard) {
                actionHandlers_["ToggleConfig"]();
                return; // Event handled, stop further processing
            }
        }

        // Handle Quit/ConfigClose (Q) when config is open
        if (*showConfig_ && (keybindProvider_->isAction(event.key, "ConfigClose") || keybindProvider_->isAction(event.key, "Quit"))) {
            // Only activate global close/quit if ImGui is NOT currently capturing keyboard input.
            // This allows 'Q' to be typed into a text field when focused in config.
            if (!io.WantCaptureKeyboard) {
                actionHandlers_["Quit"](); // Assuming "Quit" action handles closing config when applicable
                return; // Event handled, stop further processing
            }
        }
    }

    // If ImGui is currently capturing any keyboard input (e.g., a text field is active),
    // and the event hasn't been handled by a specific global keybind above,
    // then let ImGui handle the event and skip further custom InputManager processing.
    if (io.WantCaptureKeyboard) {
        return;
    }

    // Continue with original event handling for cases where ImGui is not capturing
    // keyboard input or for non-keyboard events.
    if (!*showConfig_) {
        handleDoubleClick(event);
    }

    // If config is active but ImGui is not capturing keyboard (meaning no text field
    // or other ImGui element has focus that consumes keyboard input), then regular
    // keybinds (e.g., navigation) should still work.
    if (*showConfig_) {
        handleRegularEvents(event);
    } else {
        handleRegularEvents(event);
    }
}

void InputManager::handleConfigEvents(const SDL_Event& event) {
    handleDoubleClick(event);
    if (event.type == SDL_KEYDOWN) {
        if (keybindProvider_->isAction(event.key, "ConfigClose") || 
            keybindProvider_->isAction(event.key, "Quit")) {
            actionHandlers_["Quit"]();
        }
    }
}

void InputManager::handleRegularEvents(const SDL_Event& event) {
    for (const auto& action : keybindProvider_->getActions()) {
        if (action == "ConfigClose" || action == "ScreenshotKey" || action == "ScreenshotQuit") {
            continue; // Skip config and screenshot-specific actions
        }
        if (event.type == SDL_KEYDOWN && keybindProvider_->isAction(event.key, action)) {
            auto it = actionHandlers_.find(action);
            if (it != actionHandlers_.end()) it->second();
        } else if (event.type == SDL_JOYBUTTONDOWN && keybindProvider_->isJoystickAction(event.jbutton, action)) {
            auto it = actionHandlers_.find(action);
            if (it != actionHandlers_.end()) it->second();
        } else if (event.type == SDL_JOYHATMOTION && keybindProvider_->isJoystickHatAction(event.jhat, action)) {
            auto it = actionHandlers_.find(action);
            if (it != actionHandlers_.end()) it->second();
        } else if (event.type == SDL_JOYAXISMOTION && keybindProvider_->isJoystickAxisAction(event.jaxis, action)) {
            auto it = actionHandlers_.find(action);
            if (it != actionHandlers_.end()) it->second();
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
            soundManager_->playSound("config_save");
            lastClickTimes_.erase(it); // Clear to reset double-click detection
        } else {
            lastClickTimes_[windowID] = currentTime;
        }
    }
}