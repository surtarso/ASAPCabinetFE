#include "keybinds/input_manager.h"
#include "keybinds/keybind_manager.h"
#include "utils/logging.h"
#include <random>

InputManager::InputManager(IKeybindProvider* keybindProvider)
    : keybindProvider_(keybindProvider), assets_(nullptr), soundManager_(nullptr),
      settingsManager_(nullptr), currentIndex_(nullptr), tables_(nullptr), showConfig_(nullptr),
      exeDir_(""), screenshotManager_(nullptr) {}

void InputManager::setDependencies(AssetManager* assets, ISoundManager* sound, IConfigService* settings,
                                   size_t& currentIndex, const std::vector<TableLoader>& tables,
                                   bool& showConfig, const std::string& exeDir, ScreenshotManager* screenshotManager) {
    assets_ = assets;
    soundManager_ = sound;
    settingsManager_ = settings;
    currentIndex_ = &currentIndex;  // Fixed typo: ¤tIndex → currentIndex
    tables_ = &tables;
    showConfig_ = &showConfig;
    exeDir_ = exeDir;
    screenshotManager_ = screenshotManager;  // Injected, not created

    for (size_t i = 0; i < tables_->size(); ++i) {
        if (!tables_->at(i).tableName.empty()) {
            char firstChar = tables_->at(i).tableName[0];
            char key = std::isalpha(firstChar) ? std::toupper(firstChar) : firstChar;
            if (letterIndex_.find(key) == letterIndex_.end()) {
                letterIndex_[key] = i;
            }
        }
    }
}

void InputManager::registerActions() {
    actionHandlers_["PreviousTable"] = [this]() {
        LOG_DEBUG("Previous table triggered");
        size_t newIndex = (*currentIndex_ + tables_->size() - 1) % tables_->size();
        if (newIndex != *currentIndex_) {
            assets_->loadTableAssets(newIndex, *tables_);
            *currentIndex_ = newIndex;
            soundManager_->playSound("scroll_prev");
        }
    };

    actionHandlers_["NextTable"] = [this]() {
        LOG_DEBUG("Next table triggered");
        size_t newIndex = (*currentIndex_ + 1) % tables_->size();
        if (newIndex != *currentIndex_) {
            assets_->loadTableAssets(newIndex, *tables_);
            *currentIndex_ = newIndex;
            soundManager_->playSound("scroll_next");
        }
    };

    actionHandlers_["FastPrevTable"] = [this]() {
        LOG_DEBUG("Fast previous table triggered");
        size_t newIndex = (*currentIndex_ + tables_->size() - 10) % tables_->size();
        if (newIndex != *currentIndex_) {
            assets_->loadTableAssets(newIndex, *tables_);
            *currentIndex_ = newIndex;
            soundManager_->playSound("scroll_fast_prev");
        }
    };

    actionHandlers_["FastNextTable"] = [this]() {
        LOG_DEBUG("Fast next table triggered");
        size_t newIndex = (*currentIndex_ + 10) % tables_->size();
        if (newIndex != *currentIndex_) {
            assets_->loadTableAssets(newIndex, *tables_);
            *currentIndex_ = newIndex;
            soundManager_->playSound("scroll_fast_next");
        }
    };

    actionHandlers_["JumpPrevLetter"] = [this]() {
        LOG_DEBUG("Jump previous letter triggered");
        char currentChar = tables_->at(*currentIndex_).tableName[0];
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
        LOG_DEBUG("Jump next letter triggered");
        char currentChar = tables_->at(*currentIndex_).tableName[0];
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
        LOG_DEBUG("Random table triggered");
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
        LOG_DEBUG("Launch table triggered");
        soundManager_->playSound("launch_table");
        const Settings& settings = settingsManager_->getSettings();
        std::string command = settings.vpxStartArgs + " " + settings.vpxExecutableCmd + " " +
                              settings.vpxSubCmd + " \"" + tables_->at(*currentIndex_).vpxFile + "\" " +
                              settings.vpxEndArgs;
        LOG_DEBUG("Launching: " << command);
        int result = std::system(command.c_str());
        if (result != 0) {
            std::cerr << "Warning: VPX launch failed with exit code " << result << std::endl;
        }
    };

    actionHandlers_["ScreenshotMode"] = [this]() {
        LOG_DEBUG("Screenshot mode triggered");
        if (!inScreenshotMode_) {
            soundManager_->playSound("launch_screenshot");
            inScreenshotMode_ = true;
            screenshotManager_->launchScreenshotMode(tables_->at(*currentIndex_).vpxFile);
            inScreenshotMode_ = false;
            LOG_DEBUG("Exited screenshot mode");
        }
    };

    actionHandlers_["ToggleConfig"] = [this]() {
        LOG_DEBUG("ToggleConfig action triggered");
        soundManager_->playSound("config_toggle");
        *showConfig_ = !*showConfig_;
        LOG_DEBUG("Toggled showConfig to: " << (*showConfig_ ? 1 : 0));
    };

    actionHandlers_["Quit"] = [this]() {
        LOG_DEBUG("Quit triggered");
        soundManager_->playSound("quit");
        if (inScreenshotMode_) {
            inScreenshotMode_ = false;
            LOG_DEBUG("Exited screenshot mode (quit skipped)");
        } else if (*showConfig_) {
            *showConfig_ = false;
            LOG_DEBUG("Closed config");
        } else {
            quit_ = true;
            LOG_DEBUG("Quitting app");
        }
    };

    actionHandlers_["ConfigSave"] = [this]() {
        LOG_DEBUG("ConfigSave triggered");
        if (runtimeEditor_) {
            runtimeEditor_->saveConfig();
            soundManager_->playSound("config_save");
            LOG_DEBUG("Config saved via ConfigUI");
        } else {
            LOG_DEBUG("Error: ConfigUI not set");
        }
    };

    actionHandlers_["ConfigClose"] = [this]() {
        LOG_DEBUG("ConfigClose triggered");
        *showConfig_ = false;
        soundManager_->playSound("config_close");
    };
}

void InputManager::handleEvent(const SDL_Event& event) {
    if (event.type == SDL_QUIT) {
        quit_ = true;
        LOG_DEBUG("SDL_QUIT received");
        return;
    }

    if (event.type == SDL_JOYDEVICEADDED || event.type == SDL_JOYDEVICEREMOVED) {
        return; // Handled in App
    }

    if (event.type == SDL_KEYDOWN && keybindProvider_->isAction(event.key, "ToggleConfig")) {
        actionHandlers_["ToggleConfig"]();
        return;
    }

    if (*showConfig_) {
        handleConfigEvents(event);
    } else if (!inScreenshotMode_) {
        handleRegularEvents(event);
    }
}

void InputManager::handleConfigEvents(const SDL_Event& event) {
    if (event.type == SDL_KEYDOWN) {
        if (keybindProvider_->isAction(event.key, "ConfigSave")) {
            actionHandlers_["ConfigSave"]();
        } else if (keybindProvider_->isAction(event.key, "ConfigClose") || 
                   keybindProvider_->isAction(event.key, "Quit")) {
            actionHandlers_["Quit"]();
        }
    }
}

void InputManager::handleRegularEvents(const SDL_Event& event) {
    for (const auto& action : keybindProvider_->getActions()) {
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