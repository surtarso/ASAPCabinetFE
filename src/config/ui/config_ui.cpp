/**
 * @file config_ui.cpp
 * @brief Implementation of the ConfigUI class for rendering and managing the configuration UI in ASAPCabinetFE.
 */

#include "config_ui.h"
#include "section_renderer.h"
#include "sound/isound_manager.h"
#include "log/logging.h"
#include <set>
#include <string>

ConfigUI::ConfigUI(IConfigService* configService, IKeybindProvider* keybindProvider,
                   IAssetManager* assets, [[maybe_unused]] size_t* currentIndex,
                   [[maybe_unused]] std::vector<TableData>* tables, IAppCallbacks* appCallbacks,
                   bool& showConfig, bool standaloneMode)
    : configService_(configService),
      keybindProvider_(keybindProvider),
      assets_(assets),
      appCallbacks_(appCallbacks),
      showConfig_(showConfig),
      standaloneMode_(standaloneMode),
      standaloneFileDialog_(),
      normalFileDialog_() {
    LOG_INFO("ConfigUI constructed.");
    try {
        jsonData_ = nlohmann::json(configService_->getSettings());
        if (jsonData_.contains("Keybinds")) {
            for (const auto& action : keybindProvider_->getActions()) {
                SDL_Event event;
                event.type = SDL_KEYDOWN;
                event.key.keysym.sym = keybindProvider_->getKey(action);
                event.key.keysym.scancode = SDL_SCANCODE_UNKNOWN;
                event.key.keysym.mod = 0;
                event.key.state = SDL_PRESSED;
                event.key.repeat = 0;
                std::string currentBind = keybindProvider_->eventToString(event);
                if (currentBind.empty()) {
                    if (auto key = keybindProvider_->getKey(action); key != SDLK_UNKNOWN) {
                        currentBind = SDL_GetKeyName(key);
                    }
                }
                jsonData_["Keybinds"][action] = currentBind;
            }
        }
        originalJsonData_ = jsonData_;
    } catch (const std::exception& e) {
        LOG_ERROR("Error initializing JSON data: " + std::string(e.what()));
    }
    initializeRenderers();
}

void ConfigUI::initializeRenderers() {
    for (const auto& section : sectionConfig_.getSectionOrder()) {
        renderers_[section] = std::make_unique<SectionRenderer>(this, sectionConfig_.getKeyOrder(section));
    }
}

void ConfigUI::drawGUI() {
    ImGuiIO& io = ImGui::GetIO();
    ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove |
                                   ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse;

    // Detect display orientation
    bool isLandscape = io.DisplaySize.x > io.DisplaySize.y;

    // Landscape → full-screen mode
    if (isLandscape || standaloneMode_) {
        ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(io.DisplaySize.x, io.DisplaySize.y), ImGuiCond_Always);
        ImGui::Begin("ASAPCabinetFE 1st Run Setup", &showConfig_, windowFlags);
    } else {
        float configUIWidth = 0.7f;
        float configUIHeight = 0.5f;
        try {
            if (jsonData_.contains("Internal") && jsonData_["Internal"].is_object()) {
                if (jsonData_["Internal"].contains("configUIWidth")) {
                    configUIWidth = jsonData_["Internal"]["configUIWidth"].get<float>();
                }
                if (jsonData_["Internal"].contains("configUIHeight")) {
                    configUIHeight = jsonData_["Internal"]["configUIHeight"].get<float>();
                }
            }
        } catch (const std::exception& e) {
            LOG_ERROR("Error accessing window size ratios: " + std::string(e.what()));
        }

        float configWidth = io.DisplaySize.x * configUIWidth;
        float configHeight = io.DisplaySize.y * configUIHeight;
        float configX = io.DisplaySize.x / 2.0f - configWidth / 2.0f;
        float configY = io.DisplaySize.y / 2.0f - configHeight / 2.0f;

        ImGui::SetNextWindowPos(ImVec2(configX, configY), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(configWidth, configHeight), ImGuiCond_Always);
        ImGui::Begin("ASAPCabinetFE Configuration", &showConfig_, windowFlags);
    }

    if (jsonData_.is_null()) {
        LOG_ERROR("JSON data is null.");
        ImGui::Text("Error: Failed to load configuration data.");
        ImGui::End();
        return;
    }

    bool hasChanges = jsonData_ != originalJsonData_;

    float buttonHeight = ImGui::GetFrameHeightWithSpacing() + 15.0f;
    ImGui::BeginChild("ConfigContent", ImVec2(0, -buttonHeight), false, ImGuiWindowFlags_AlwaysVerticalScrollbar);
    std::set<std::string> renderedSections;

    ImGuiFileDialog* fileDialog = standaloneMode_ ? &standaloneFileDialog_ : &normalFileDialog_;

    if (standaloneMode_) {
        if (jsonData_.contains("VPX")) {
            ImGui::PushID("VPX");
            auto it = renderers_.find("VPX");
            if (it != renderers_.end()) {
                it->second->render("VPX", jsonData_["VPX"], isCapturingKey_, capturingKeyName_, fileDialog, true, isDialogOpen_, dialogKey_);
                if (ImGui::Button("Reset to Default", ImVec2(120, 0))) {
                    resetSectionToDefault("VPX");
                }
            } else {
                LOG_ERROR("No renderer for section VPX");
            }
            renderedSections.insert("VPX");
            ImGui::PopID();
            ImGui::Spacing();
        }
        if (jsonData_.contains("WindowSettings")) {
            ImGui::PushID("WindowSettings");
            auto it = renderers_.find("WindowSettings");
            if (it != renderers_.end()) {
                it->second->render("WindowSettings", jsonData_["WindowSettings"], isCapturingKey_, capturingKeyName_, fileDialog, false, isDialogOpen_, dialogKey_);
            } else {
                LOG_ERROR("No renderer for section WindowSettings");
            }
            renderedSections.insert("WindowSettings");
            ImGui::PopID();
            ImGui::Spacing();
        }
    } else {
        for (const auto& sectionName : sectionConfig_.getSectionOrder()) {
            if (jsonData_.contains(sectionName)) {
                ImGui::PushID(sectionName.c_str());
                auto it = renderers_.find(sectionName);
                if (it != renderers_.end()) {
                    bool defaultOpen = false;
                    // if (sectionName == "Keybinds" && isCapturingKey_) {
                    //     ImGui::Text("Press a key or joystick input to bind to %s...", capturingKeyName_.c_str());
                    // }
                    it->second->render(sectionName, jsonData_[sectionName], isCapturingKey_, capturingKeyName_, fileDialog, defaultOpen, isDialogOpen_, dialogKey_);
                } else {
                    LOG_ERROR("No renderer for section " + sectionName);
                }
                renderedSections.insert(sectionName);
                ImGui::PopID();
                ImGui::Spacing();
            }
        }
    }

    ImGui::EndChild();
    ImGui::SetCursorPosY(ImGui::GetWindowHeight() - buttonHeight);

    ImGui::Separator();
    if (hasChanges) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.8f, 0.2f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.9f, 0.3f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.1f, 0.7f, 0.1f, 1.0f));
    }
    if (ImGui::Button("Apply", ImVec2(100, 0))) {
        saveConfig();
        if (standaloneMode_) {
            showConfig_ = false;
        }
    }
    ImGui::SameLine();
    if (hasChanges) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.3f, 0.3f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.7f, 0.1f, 0.1f, 1.0f));
    }
    if (ImGui::Button("Close", ImVec2(100, 0))) {
        jsonData_ = originalJsonData_;
        if (standaloneMode_) {
            LOG_WARN("Quit without saving.");
            exit(1);
        } else {
            showConfig_ = false;
        }
    }
    if (hasChanges) {
        ImGui::PopStyleColor(3);
        ImGui::PopStyleColor(3);
    }

    if (isDialogOpen_) {
        LOG_DEBUG("Attempting to display dialog for key: " + dialogKey_ + ", isDialogOpen_: " + std::to_string(isDialogOpen_));
        ImVec2 maxSize = ImVec2(ImGui::GetIO().DisplaySize.x * 0.8f, ImGui::GetIO().DisplaySize.y * 0.8f);
        ImVec2 minSize = ImVec2(600, 400);

        // VPX SECTION
        if (dialogKey_ == "VPXTablesPath") {
            if (fileDialog->Display("FolderDlg_VPXTablesPath", ImGuiWindowFlags_NoCollapse, minSize, maxSize)) {
                LOG_DEBUG("Displaying FolderDlg_VPXTablesPath");
                if (fileDialog->IsOk()) {
                    jsonData_["VPX"]["VPXTablesPath"] = fileDialog->GetCurrentPath();
                    LOG_INFO("Selected VPXTablesPath: " + jsonData_["VPX"]["VPXTablesPath"].get<std::string>());
                }
                fileDialog->Close();
                isDialogOpen_ = false;
            }
        } else if (dialogKey_ == "VPinballXPath") {
            if (fileDialog->Display("FileDlg_VPinballXPath", ImGuiWindowFlags_NoCollapse, minSize, maxSize)) {
                LOG_DEBUG("Displaying FileDlg_VPinballXPath");
                if (fileDialog->IsOk()) {
                    jsonData_["VPX"]["VPinballXPath"] = fileDialog->GetFilePathName();
                    LOG_INFO("Selected VPinballXPath: " + jsonData_["VPX"]["VPinballXPath"].get<std::string>());
                }
                fileDialog->Close();
                isDialogOpen_ = false;
            }
        } else if (dialogKey_ == "vpxIniPath") {
            if (fileDialog->Display("FileDlg_vpxIniPath", ImGuiWindowFlags_NoCollapse, minSize, maxSize)) {
                LOG_DEBUG("Displaying FileDlg_vpxIniPath");
                if (fileDialog->IsOk()) {
                    jsonData_["VPX"]["vpxIniPath"] = fileDialog->GetFilePathName();
                    LOG_INFO("Selected vpxIniPath: " + jsonData_["VPX"]["vpxIniPath"].get<std::string>());
                }
                fileDialog->Close();
                isDialogOpen_ = false;
            }

        // UI SOUNDS SECTION
        } else if (dialogKey_ == "scrollNormalSound" || dialogKey_ == "scrollFastSound" ||
                    dialogKey_ == "scrollJumpSound" || dialogKey_ == "scrollRandomSound" ||
                    dialogKey_ == "launchTableSound" || dialogKey_ == "launchScreenshotSound" ||
                    dialogKey_ == "panelToggleSound" || dialogKey_ == "screenshotTakeSound" ||
                    dialogKey_ == "ambienceSound") {

            if (fileDialog->Display("FileDlg_AudioPath", ImGuiWindowFlags_NoCollapse, minSize, maxSize)) {
                LOG_DEBUG("Displaying FileDlg_AudioPath");
                if (fileDialog->IsOk()) {
                    // Using dialogKey_ to identify the specific key (e.g., "interfaceAudioVol")
                    jsonData_["UISounds"][dialogKey_] = fileDialog->GetFilePathName();
                    LOG_INFO("Selected " + dialogKey_ + ": " + jsonData_["UISounds"][dialogKey_].get<std::string>());
                }
                fileDialog->Close();
                isDialogOpen_ = false;
            }

        // MEDIA SECTION
        } else if (dialogKey_ == "defaultPlayfieldImage" || dialogKey_ == "defaultBackglassImage" ||
                    dialogKey_ == "defaultDmdImage" || dialogKey_ == "defaultWheelImage" ||
                    dialogKey_ == "defaultTopperImage" || dialogKey_ == "defaultPlayfieldVideo" ||
                    dialogKey_ == "defaultBackglassVideo" || dialogKey_ == "defaultDmdVideo" ||
                    dialogKey_ == "defaultTopperVideo" || dialogKey_ == "customPlayfieldImage" ||
                    dialogKey_ == "customBackglassImage" || dialogKey_ == "customDmdImage" ||
                    dialogKey_ == "customWheelImage" || dialogKey_ == "customTopperImage" ||
                    dialogKey_ == "customPlayfieldVideo" || dialogKey_ == "customBackglassVideo" ||
                    dialogKey_ == "customDmdVideo" || dialogKey_ == "customTopperVideo" ||
                    dialogKey_ == "tableMusic" || dialogKey_ == "customLaunchSound") {

            // 1. Image Dialog
            if (fileDialog->IsOpened("FileDlg_ImagePath")) {
                if (fileDialog->Display("FileDlg_ImagePath", ImGuiWindowFlags_NoCollapse, minSize, maxSize)) {
                    if (fileDialog->IsOk()) {
                        // Determine the correct section based on the dialogKey_
                        std::string targetSection = (dialogKey_.find("custom") != std::string::npos) ? "CustomMedia" : "DefaultMedia";
                        jsonData_[targetSection][dialogKey_] = fileDialog->GetFilePathName();
                        LOG_INFO("Selected " + dialogKey_ + ": " + jsonData_[targetSection][dialogKey_].get<std::string>());
                    }
                    fileDialog->Close();
                    isDialogOpen_ = false;
                }
            }

            // 2. Video Dialog
            else if (fileDialog->IsOpened("FileDlg_VideoPath")) {
                if (fileDialog->Display("FileDlg_VideoPath", ImGuiWindowFlags_NoCollapse, minSize, maxSize)) {
                    if (fileDialog->IsOk()) {
                        // Determine the correct section
                        std::string targetSection = (dialogKey_.find("custom") != std::string::npos) ? "CustomMedia" : "DefaultMedia";
                        jsonData_[targetSection][dialogKey_] = fileDialog->GetFilePathName();
                        LOG_INFO("Selected " + dialogKey_ + ": " + jsonData_[targetSection][dialogKey_].get<std::string>());
                    }
                    fileDialog->Close();
                    isDialogOpen_ = false;
                }
            }

            // 3. Audio Dialog
            else if (fileDialog->IsOpened("FileDlg_AudioPath")) {
                if (fileDialog->Display("FileDlg_AudioPath", ImGuiWindowFlags_NoCollapse, minSize, maxSize)) {
                    if (fileDialog->IsOk()) {
                        // Determine the correct section
                        std::string targetSection = (dialogKey_.find("custom") != std::string::npos) ? "CustomMedia" : "DefaultMedia";
                        jsonData_[targetSection][dialogKey_] = fileDialog->GetFilePathName();
                        LOG_INFO("Selected " + dialogKey_ + ": " + jsonData_[targetSection][dialogKey_].get<std::string>());
                    }
                    fileDialog->Close();
                    isDialogOpen_ = false;
                }
            }
        } else {
            LOG_ERROR("Unknown dialog key: " + dialogKey_);
            isDialogOpen_ = false;
        }
    }

    ImGui::End();
}

void ConfigUI::handleEvent(const SDL_Event& event) {
    if (!isCapturingKey_) return;

    if (event.type == SDL_KEYDOWN || event.type == SDL_JOYBUTTONDOWN ||
        event.type == SDL_JOYHATMOTION || event.type == SDL_JOYAXISMOTION) {
        std::string newBind = keybindProvider_->eventToString(event);
        if (!newBind.empty()) {
            LOG_DEBUG("Captured bind: " + newBind + " for " + capturingKeyName_);
            updateKeybind(capturingKeyName_, newBind);
            isCapturingKey_ = false;
        }
    }
}

void ConfigUI::updateKeybind(const std::string& action, const std::string& bind) {
    if (!jsonData_.contains("Keybinds") || !keybindProvider_) return;

    std::string storedBind = bind;

    jsonData_["Keybinds"][action] = storedBind;
    SDL_Keycode key = SDL_GetKeyFromName(bind.c_str());
    if (key != SDLK_UNKNOWN) {
        keybindProvider_->setKey(action, key);
        LOG_DEBUG("Updated keybind " + action + " to " + storedBind + " (keycode: " + std::to_string(key) + ")");
    } else if (bind.find("JOY_") == 0) {
        try {
            if (bind.find("_BUTTON_") != std::string::npos) {
                size_t joyEnd = bind.find("_BUTTON_");
                int joystickId = std::stoi(bind.substr(4, joyEnd - 4));
                uint8_t button = static_cast<uint8_t>(std::stoi(bind.substr(joyEnd + 8)));
                keybindProvider_->setJoystickButton(action, joystickId, button);
            } else if (bind.find("_HAT_") != std::string::npos) {
                size_t joyEnd = bind.find("_HAT_");
                size_t hatEnd = bind.find("_", joyEnd + 5);
                int joystickId = std::stoi(bind.substr(4, joyEnd - 4));
                uint8_t hat = static_cast<uint8_t>(std::stoi(bind.substr(joyEnd + 5, hatEnd - (joyEnd + 5))));
                std::string directionStr = bind.substr(hatEnd + 1);
                uint8_t direction = SDL_HAT_CENTERED;
                if (directionStr == "UP") direction = SDL_HAT_UP;
                else if (directionStr == "DOWN") direction = SDL_HAT_DOWN;
                else if (directionStr == "LEFT") direction = SDL_HAT_LEFT;
                else if (directionStr == "RIGHT") direction = SDL_HAT_RIGHT;
                keybindProvider_->setJoystickHat(action, joystickId, hat, direction);
            } else if (bind.find("_AXIS_") != std::string::npos) {
                size_t joyEnd = bind.find("_AXIS_");
                size_t axisEnd = bind.find("_", joyEnd + 6);
                int joystickId = std::stoi(bind.substr(4, joyEnd - 4));
                uint8_t axis = static_cast<uint8_t>(std::stoi(bind.substr(joyEnd + 6, axisEnd - (joyEnd + 6))));
                bool positiveDirection = (bind.substr(axisEnd + 1) == "POSITIVE");
                keybindProvider_->setJoystickAxis(action, joystickId, axis, positiveDirection);
            }
            LOG_DEBUG("Updated joystick bind " + action + " to " + bind);
        } catch (const std::exception& e) {
            LOG_ERROR("Invalid joystick bind format for " + action + ": " + bind + ", error: " + std::string(e.what()));
        }
    } else {
        LOG_ERROR("Invalid keybind " + bind + " for action " + action);
    }
}

void ConfigUI::saveConfig() {
    LOG_DEBUG("saveConfig called.");
    if (!configService_ || jsonData_.is_null()) {
        LOG_ERROR("Cannot save config, service or JSON data is null.");
        return;
    }

    try {
        std::set<Settings::ReloadType> reloadTypes;
        nlohmann::json originalSettings = nlohmann::json(configService_->getSettings());

        for (const auto& [sectionName, sectionData] : jsonData_.items()) {
            for (const auto& [key, newValue] : sectionData.items()) {
                auto originalValue = originalSettings.value(sectionName, nlohmann::json{}).value(key, nlohmann::json{});
                if (newValue != originalValue) {
                    std::string fullKey = key;
                    auto it = Settings::settingsMetadata.find(fullKey);
                    if (it != Settings::settingsMetadata.end()) {
                        reloadTypes.insert(it->second.first);
                        LOG_DEBUG("Detected change in " + sectionName + "." + key + ", ReloadType: " + std::to_string(static_cast<int>(it->second.first)));
                    } else {
                        LOG_DEBUG("No ReloadType found for " + sectionName + "." + key);
                    }
                }
            }
        }

        Settings& settings = const_cast<Settings&>(configService_->getSettings());
        settings = jsonData_;

        if (jsonData_.contains("Keybinds") && jsonData_["Keybinds"].is_object()) {
            std::map<std::string, std::string> keybindData;
            for (const auto& [action, bind] : jsonData_["Keybinds"].items()) {
                if (bind.is_string()) {
                    keybindData[action] = bind.get<std::string>();
                }
            }
            keybindProvider_->loadKeybinds(keybindData);
            LOG_DEBUG("Synced keybinds to KeybindManager");
        }

        if (settings.forceRebuildMetadata && settings.ignoreScanners) {
            LOG_WARN("User wants to rebuild but Ignore Scanners is also true — disabling ignoreScanners to ensure rebuild runs.");
            settings.ignoreScanners = false;
        }

        configService_->saveConfig();
        LOG_DEBUG("Config saved successfully.");

        for (const auto& reloadType : reloadTypes) {
            switch (reloadType) {
                case Settings::ReloadType::None:
                    break;
                case Settings::ReloadType::Title:
                    if (assets_) {
                        assets_->setTitlePosition(settings.titleX, settings.titleY);
                        LOG_DEBUG("Triggered setTitlePosition for ReloadType " + std::to_string(static_cast<int>(reloadType)));
                    }
                    break;
                case Settings::ReloadType::Font:
                    if (appCallbacks_) {
                        appCallbacks_->reloadFont(standaloneMode_);
                        LOG_DEBUG("Triggered reloadFont for ReloadType " + std::to_string(static_cast<int>(reloadType)));
                    }
                    break;
                case Settings::ReloadType::Windows:
                    if (appCallbacks_) {
                        appCallbacks_->reloadWindows();
                        LOG_DEBUG("Triggered reloadWindows");
                    }
                    break;
                case Settings::ReloadType::Assets:
                    if (appCallbacks_) {
                        appCallbacks_->reloadAssetsAndRenderers();
                        LOG_DEBUG("Triggered reloadAssetsAndRenderers for ReloadType " + std::to_string(static_cast<int>(reloadType)));
                    }
                    break;
                case Settings::ReloadType::Tables:
                    if (appCallbacks_) {
                        appCallbacks_->reloadTablesAndTitle();
                        LOG_DEBUG("Triggered reloadTablesAndTitle for ReloadType " + std::to_string(static_cast<int>(reloadType)));
                    }
                    break;
                case Settings::ReloadType::Overlay:
                    if (appCallbacks_) {
                        appCallbacks_->reloadOverlaySettings();
                        LOG_DEBUG("Triggered reloadOverlaySettings");
                    }
                    break;
                case Settings::ReloadType::Audio:
                    if (appCallbacks_) {
                        ISoundManager* soundManager = appCallbacks_->getSoundManager();
                        if (soundManager) {
                            soundManager->updateSettings(configService_->getSettings());
                            LOG_DEBUG("AudioSettings changed and saved, updated ISoundManager");
                        }
                        if (assets_) {
                            assets_->applyVideoAudioSettings();
                            LOG_DEBUG("AudioSettings changed and saved, updated AssetManager");
                        }
                    }
                    break;
                default:
                    LOG_ERROR("Unknown ReloadType " + std::to_string(static_cast<int>(reloadType)));
                    break;
            }
        }

        originalJsonData_ = jsonData_;
    } catch (const std::exception& e) {
        LOG_ERROR("Error saving config: " + std::string(e.what()));
    }
}

void ConfigUI::resetSectionToDefault(const std::string& sectionName) {
    LOG_DEBUG("Resetting section " + sectionName + " to default.");
    Settings defaultSettings;
    nlohmann::json defaultJson;
    to_json(defaultJson, defaultSettings);

    if (defaultJson.contains(sectionName)) {
        jsonData_[sectionName] = defaultJson[sectionName];
        LOG_INFO("Section " + sectionName + " reset to default values.");
    } else {
        LOG_ERROR("No default data found for section " + sectionName);
    }
}

void ConfigUI::refreshUIState() {
    LOG_DEBUG("Refreshing UI state");
    try {
        jsonData_ = nlohmann::json(configService_->getSettings());
        originalJsonData_ = jsonData_;
        LOG_INFO("UI state refreshed successfully");
    } catch (const std::exception& e) {
        LOG_ERROR("Error refreshing UI state: " + std::string(e.what()));
    }
}
