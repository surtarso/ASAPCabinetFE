/**
 * @file input_manager.cpp
 * @brief Implementation of the InputManager class for handling user input and keybindings in ASAPCabinetFE.
 */

#include "input_manager.h"
#include "log/logging.h"
#include "imgui.h"
#include "backends/imgui_impl_sdl2.h"
#include <random>
#include <SDL.h>
#include <chrono>
#include <string>

InputManager::InputManager(IKeybindProvider* keybindProvider)
    : keybindProvider_(keybindProvider), assets_(nullptr), soundManager_(nullptr),
      settingsManager_(nullptr), windowManager_(nullptr), currentIndex_(nullptr),
      tables_(nullptr), showConfig_(nullptr), showEditor_(nullptr), showVpsdb_(nullptr),
      exeDir_(""), screenshotManager_(nullptr), runtimeEditor_(nullptr), actionHandlers_(),
      letterIndex_(), quit_(false), screenshotModeActive_(false), lastClickTimes_(),
      inExternalAppMode_(false), lastExternalAppReturnTime_(0), tableLauncher_(nullptr) {
    registerActions();
    LOG_INFO("InputManager constructed.");
}

void InputManager::setDependencies(IAssetManager* assets, ISoundManager* sound, IConfigService* settings,
                                  size_t& currentIndex, std::vector<TableData>& tables,
                                  bool& showConfig, bool& showEditor, bool& showVpsdb, const std::string& exeDir,
                                  IScreenshotManager* screenshotManager, IWindowManager* windowManager,
                                  std::atomic<bool>& isLoadingTables, ITableLauncher* tableLauncher,
                                  ITableCallbacks* tableCallbacks) {
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
    tableCallbacks_ = tableCallbacks;
    LOG_DEBUG("InputManager dependencies set, table count: " + std::to_string(tables_->size()));
}

void InputManager::registerActions() {
    actionHandlers_["Previous Table"] = [this]() {
        LOG_DEBUG("Previous table triggered");
        if (!tables_ || !currentIndex_) return;
        size_t newIndex = (*currentIndex_ + tables_->size() - 1) % tables_->size();
        if (newIndex != *currentIndex_) {
            assets_->loadTableAssets(newIndex, *tables_);
            *currentIndex_ = newIndex;
            soundManager_->playUISound("scroll_normal");
        }
    };

    actionHandlers_["Next Table"] = [this]() {
        LOG_DEBUG("Next table triggered");
        if (!tables_ || !currentIndex_) return;
        size_t newIndex = (*currentIndex_ + 1) % tables_->size();
        if (newIndex != *currentIndex_) {
            assets_->loadTableAssets(newIndex, *tables_);
            *currentIndex_ = newIndex;
            soundManager_->playUISound("scroll_normal");
        }
    };

    actionHandlers_["Fast Previous Table"] = [this]() {
        LOG_DEBUG("Fast previous table triggered");
        if (!tables_ || !currentIndex_) return;
        size_t newIndex = (*currentIndex_ + tables_->size() - 10) % tables_->size();
        if (newIndex != *currentIndex_) {
            assets_->loadTableAssets(newIndex, *tables_);
            *currentIndex_ = newIndex;
            soundManager_->playUISound("scroll_fast");
        }
    };

    actionHandlers_["Fast Next Table"] = [this]() {
        LOG_DEBUG("Fast next table triggered");
        if (!tables_ || !currentIndex_) return;
        size_t newIndex = (*currentIndex_ + 10) % tables_->size();
        if (newIndex != *currentIndex_) {
            assets_->loadTableAssets(newIndex, *tables_);
            *currentIndex_ = newIndex;
            soundManager_->playUISound("scroll_fast");
        }
    };

    actionHandlers_["Jump Previous Letter"] = [this]() {
        LOG_DEBUG("Jump previous letter triggered");
        if (!tables_ || tables_->empty() || !currentIndex_) {
            LOG_ERROR("Invalid tables or currentIndex for JumpPrevLetter");
            return;
        }
        size_t currentIdx = *currentIndex_;
        if (currentIdx >= tables_->size()) {
            LOG_ERROR("currentIndex " + std::to_string(currentIdx) + " out of range (size=" + std::to_string(tables_->size()) + ")");
            return;
        }
        std::string currentTitle = tables_->at(currentIdx).title;
        if (currentTitle.empty()) {
            LOG_ERROR("Empty title at index " + std::to_string(currentIdx));
            return;
        }
        char currentChar = currentTitle[0];
    char currentKey = (std::isalpha(currentChar) || std::isdigit(currentChar)) ? static_cast<char>(std::toupper(static_cast<unsigned char>(currentChar))) : '\0';
        if (currentKey == '\0') {
            LOG_ERROR("Invalid first character in title: " + currentTitle);
            return;
        }
        size_t newIndex = currentIdx;
        bool found = false;
        for (size_t i = currentIdx; i > 0; --i) {
            size_t idx = i - 1;
            std::string title = tables_->at(idx).title;
            if (title.empty()) continue;
            char c = title[0];
            char key = (std::isalpha(c) || std::isdigit(c)) ? static_cast<char>(std::toupper(static_cast<unsigned char>(c))) : '\0';
            if (key != '\0' && key < currentKey) {
                newIndex = idx;
                found = true;
                break;
            }
        }
        if (!found) {
            for (size_t i = tables_->size(); i > 0; --i) {
                size_t idx = i - 1;
                std::string title = tables_->at(idx).title;
                if (title.empty()) continue;
                char c = title[0];
                char key = (std::isalpha(c) || std::isdigit(c)) ? static_cast<char>(std::toupper(static_cast<unsigned char>(c))) : '\0';
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
            LOG_DEBUG("No previous letter found for key " + std::string(1, currentKey));
        }
    };

    actionHandlers_["Jump Next Letter"] = [this]() {
        LOG_DEBUG("Jump next letter triggered");
        if (!tables_ || tables_->empty() || !currentIndex_) {
            LOG_ERROR("Invalid tables or currentIndex for JumpNextLetter");
            return;
        }
        size_t currentIdx = *currentIndex_;
        if (currentIdx >= tables_->size()) {
            LOG_ERROR("currentIndex " + std::to_string(currentIdx) + " out of range (size=" + std::to_string(tables_->size()) + ")");
            return;
        }
        std::string currentTitle = tables_->at(currentIdx).title;
        if (currentTitle.empty()) {
            LOG_ERROR("Empty title at index " + std::to_string(currentIdx));
            return;
        }
        char currentChar = currentTitle[0];
    char currentKey = (std::isalpha(currentChar) || std::isdigit(currentChar)) ? static_cast<char>(std::toupper(static_cast<unsigned char>(currentChar))) : '\0';
        if (currentKey == '\0') {
            LOG_ERROR("Invalid first character in title: " + currentTitle);
            return;
        }
        size_t newIndex = currentIdx;
        bool found = false;
        for (size_t i = currentIdx + 1; i < tables_->size(); ++i) {
            std::string title = tables_->at(i).title;
            if (title.empty()) continue;
            char c = title[0];
            char key = (std::isalpha(c) || std::isdigit(c)) ? static_cast<char>(std::toupper(static_cast<unsigned char>(c))) : '\0';
            if (key != '\0' && key > currentKey) {
                newIndex = i;
                found = true;
                break;
            }
        }
        if (!found) {
            for (size_t i = 0; i < tables_->size(); ++i) {
                std::string title = tables_->at(i).title;
                if (title.empty()) continue;
                char c = title[0];
                char key = (std::isalpha(c) || std::isdigit(c)) ? static_cast<char>(std::toupper(static_cast<unsigned char>(c))) : '\0';
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
            LOG_DEBUG("No next letter found for key " + std::string(1, currentKey));
        }
    };

    actionHandlers_["Random Table"] = [this]() {
        LOG_DEBUG("Random table triggered");
        if (!tables_ || tables_->empty()) return;
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<size_t> dist(0, tables_->size() - 1);
        size_t newIndex = dist(gen);
        if (newIndex != *currentIndex_) {
            assets_->loadTableAssets(newIndex, *tables_);
            *currentIndex_ = newIndex;
            soundManager_->playUISound("success");
        }
    };

    actionHandlers_["Launch Table"] = [this]() {
        if (inExternalAppMode_) {
            LOG_DEBUG("Launch skipped, already in external app mode.");
            return;
        }
        Uint32 currentTime = SDL_GetTicks();
        if ((currentTime - lastExternalAppReturnTime_) < EXTERNAL_APP_DEBOUNCE_TIME_MS) {
            LOG_DEBUG("Launch skipped, debouncing after external app return.");
            return;
        }
        if (!tableLauncher_ || !tables_ || !currentIndex_) {
            LOG_ERROR("Cannot launch table, tableLauncher_ or tables_ or currentIndex_ is null");
            return;
        }
        inExternalAppMode_ = true;
        LOG_DEBUG("Launch table triggered");
        soundManager_->stopMusic();
        if (IVideoPlayer* player = assets_->getPlayfieldVideoPlayer()) {
            player->stop();
            LOG_DEBUG("Stopped playfield video player");
        }
        if (IVideoPlayer* player = assets_->getBackglassVideoPlayer()) {
            player->stop();
            LOG_DEBUG("Stopped backglass video player");
        }
        if (IVideoPlayer* player = assets_->getDmdVideoPlayer()) {
            player->stop();
            LOG_DEBUG("Stopped DMD video player");
        }
        if (IVideoPlayer* player = assets_->getTopperVideoPlayer()) {
            player->stop();
            LOG_DEBUG("Stopped topper video player");
        }
        if (tables_->at(*currentIndex_).launchAudio.empty()) {
            soundManager_->playUISound("launch_table");
        } else {
            soundManager_->playCustomLaunch(tables_->at(*currentIndex_).launchAudio);
        }

        // --- LAUNCH ASYNC ---
        // capture currentIndex by value to avoid races if user navigates
        size_t idxx = *currentIndex_;
        if (idxx > static_cast<size_t>(std::numeric_limits<int>::max())) {
            LOG_ERROR("currentIndex_ too large to fit into int");
            return;   // or clamp, or handle differently
        }
        const int launchedIndex = static_cast<int>(idxx);

        tableLauncher_->launchTableAsync(
            tables_->at(launchedIndex),
            [this, launchedIndex](int result, float timePlayed)
            {
                // We are back from external app
                inExternalAppMode_ = false;
                lastExternalAppReturnTime_ = SDL_GetTicks();

                // Safety: verify index still valid
                if (!tables_ || launchedIndex < 0 || launchedIndex >= static_cast<int>(tables_->size())) {
                    LOG_ERROR("Launch callback: table index out of range, skipping update");
                    return;
                }

                TableData& t = tables_->at(launchedIndex);

                // SUCCESS if result == 0 (mapped by launcher). Do NOT depend on previous t.isBroken.
                if (result == 0) {
                    // clear broken flag on good exit and update stats
                    t.isBroken = false;
                    t.playCount++;
                    t.playTimeLast = timePlayed;
                    t.playTimeTotal += timePlayed;
                    LOG_DEBUG("Updated TableData for " + t.title +
                            ": playCount=" + std::to_string(t.playCount) +
                            ", playTimeLast=" + std::to_string(t.playTimeLast) +
                            ", playTimeTotal=" + std::to_string(t.playTimeTotal));
                } else {
                    // non-zero mapped result -> treat as broken
                    t.isBroken = true;
                    LOG_DEBUG("Marked table " + t.title +
                            " as broken due to mapped exit code " + std::to_string(result));
                }

                // Persist changes
                if (tableCallbacks_) {
                    if (tableCallbacks_->save(settingsManager_->getSettings(), *tables_, nullptr)) {
                        LOG_DEBUG("Table data updated via callback");
                    } else {
                        LOG_ERROR("Failed to update table data via callback");
                    }
                } else {
                    LOG_ERROR("Cannot update table data, tableCallbacks_ is null");
                }

                // Resume FE media
                soundManager_->playTableMusic(t.music);

                if (IVideoPlayer* p = assets_->getPlayfieldVideoPlayer()) p->play();
                if (IVideoPlayer* p = assets_->getBackglassVideoPlayer()) p->play();
                if (IVideoPlayer* p = assets_->getDmdVideoPlayer()) p->play();
                if (IVideoPlayer* p = assets_->getTopperVideoPlayer()) p->play();

                if (result != 0) {
                    LOG_ERROR("VPX launch mapped to failure with exit code " + std::to_string(result));
                }
            }
        );

    };

    actionHandlers_["Screenshot Mode"] = [this]() {
        if (inExternalAppMode_) {
            LOG_DEBUG("Screenshot mode skipped, already in external app mode.");
            return;
        }
        Uint32 currentTime = SDL_GetTicks();
        if ((currentTime - lastExternalAppReturnTime_) < EXTERNAL_APP_DEBOUNCE_TIME_MS) {
            LOG_DEBUG("Screenshot mode skipped, debouncing after external app return.");
            return;
        }
        LOG_DEBUG("Screenshot mode triggered");
        if (!screenshotModeActive_ && screenshotManager_) {
            soundManager_->playUISound("launch_screenshot");
            screenshotModeActive_ = true;
            inExternalAppMode_ = true;
            screenshotManager_->launchScreenshotMode(tables_->at(*currentIndex_).vpxFile);
            inExternalAppMode_ = false;
            screenshotModeActive_ = false;
            lastExternalAppReturnTime_ = SDL_GetTicks();
            LOG_DEBUG("Exited screenshot mode");
        }
    };

    actionHandlers_["Toggle Config"] = [this]() {
        LOG_DEBUG("ToggleConfig action triggered");
        if (showConfig_) {
            soundManager_->playUISound("panel_toggle");
            *showConfig_ = !*showConfig_;
            LOG_DEBUG("Toggled showConfig to: " + std::to_string(*showConfig_));
        }
    };

    actionHandlers_["Toggle Editor"] = [this]() {
        LOG_DEBUG("ToggleEditor action triggered");
        if (showEditor_) {
            soundManager_->playUISound("panel_toggle");
            *showEditor_ = !*showEditor_;
            LOG_DEBUG("Toggled showEditor to: " + std::to_string(*showEditor_));
        }
    };

    actionHandlers_["Toggle Catalog"] = [this]() {
        LOG_DEBUG("ToggleCatalog action triggered");
        if (showVpsdb_) {
            soundManager_->playUISound("panel_toggle");
            *showVpsdb_ = !*showVpsdb_;
            LOG_DEBUG("Toggled showVpsdb to: " + std::to_string(*showVpsdb_));
        }
    };

    actionHandlers_["Quit"] = [this]() {
        LOG_DEBUG("Quit triggered");
        if (screenshotModeActive_) {
            screenshotModeActive_ = false;
            LOG_DEBUG("Exited screenshot mode (quit skipped)");
        } else if (*showConfig_) {
            *showConfig_ = false;
            LOG_DEBUG("Closed Config UI");
        } else if (*showEditor_) {
            *showEditor_ = false;
            LOG_DEBUG("Closed Editor");
        } else if (*showVpsdb_) {
            *showVpsdb_ = false;
            LOG_DEBUG("Closed Catalog");
        } else {
            quit_ = true;
            LOG_INFO("Quitting app");
        }
    };
}

void InputManager::handleEvent(const SDL_Event& event) {
    if (event.type == SDL_QUIT) {
        quit_ = true;
        LOG_INFO("SDL_QUIT received");
        return;
    }

    if (event.type == SDL_JOYDEVICEADDED || event.type == SDL_JOYDEVICEREMOVED) {
        return;
    }

    // ImGui_ImplSDL2_ProcessEvent(&event); // handled by imGui first??

    Uint32 currentTime = SDL_GetTicks();
    ImGuiIO& io = ImGui::GetIO();

    if (isLoadingTables_ && *isLoadingTables_) {
        if (event.type == SDL_KEYDOWN && keybindProvider_->isAction(event.key, "Quit")) {
            actionHandlers_["Quit"]();
        }
        return;
    }

    if (inExternalAppMode_ || screenshotManager_->isActive() ||
        (currentTime - lastExternalAppReturnTime_) < EXTERNAL_APP_DEBOUNCE_TIME_MS) {
        return;
    }

    if (event.type == SDL_KEYDOWN && !io.WantCaptureKeyboard) {
        if (keybindProvider_->isAction(event.key, "Toggle Config")) {
            actionHandlers_["Toggle Config"]();
            return;
        }
        if (keybindProvider_->isAction(event.key, "Toggle Editor")) {
            actionHandlers_["Toggle Editor"]();
            return;
        }
        if (keybindProvider_->isAction(event.key, "Toggle Catalog")) {
            actionHandlers_["Toggle Catalog"]();
            return;
        }
    }

    if (*showConfig_) {
        if (event.type == SDL_KEYDOWN && !io.WantCaptureKeyboard) {
            if (keybindProvider_->isAction(event.key, "Toggle Config") || keybindProvider_->isAction(event.key, "Quit")) {
                actionHandlers_["Quit"]();
                return;
            }
        }
        return;
    }

    if (*showEditor_) {
        if (event.type == SDL_KEYDOWN && !io.WantCaptureKeyboard) {
            if (keybindProvider_->isAction(event.key, "Toggle Editor") || keybindProvider_->isAction(event.key, "Quit")) {
                actionHandlers_["Quit"]();
                return;
            }
            if (keybindProvider_->isAction(event.key, "Previous Table")) {
                actionHandlers_["Previous Table"]();
                return;
            }
            if (keybindProvider_->isAction(event.key, "Next Table")) {
                actionHandlers_["Next Table"]();
                return;
            }
        }
        return;
    }

    if (*showVpsdb_) {
        if (event.type == SDL_KEYDOWN && !io.WantCaptureKeyboard) {
            if (keybindProvider_->isAction(event.key, "Toggle Catalog") || keybindProvider_->isAction(event.key, "Quit")) {
                actionHandlers_["Quit"]();
                return;
            }
        }
        return;
    }

    if (io.WantCaptureKeyboard) {
        return;
    }

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
                LOG_DEBUG("Triggering action: " + action);
                it->second();
                return;
            }
        } else if (event.type == SDL_JOYBUTTONDOWN && keybindProvider_->isJoystickAction(event.jbutton, action)) {
            auto it = actionHandlers_.find(action);
            if (it != actionHandlers_.end()) {
                LOG_DEBUG("Triggering joystick action: " + action);
                it->second();
                return;
            }
        } else if (event.type == SDL_JOYHATMOTION && keybindProvider_->isJoystickHatAction(event.jhat, action)) {
            auto it = actionHandlers_.find(action);
            if (it != actionHandlers_.end()) {
                LOG_DEBUG("Triggering joystick hat action: " + action);
                it->second();
                return;
            }
        } else if (event.type == SDL_JOYAXISMOTION && keybindProvider_->isJoystickAxisAction(event.jaxis, action)) {
            auto it = actionHandlers_.find(action);
            if (it != actionHandlers_.end()) {
                LOG_DEBUG("Triggering joystick axis action: " + action);
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
            LOG_DEBUG("Double-click detected on window ID: " + std::to_string(windowID));
            if (windowManager_ && settingsManager_) {
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
            }
            lastClickTimes_.erase(it);
        } else {
            lastClickTimes_[windowID] = currentTime;
        }
    }
}
