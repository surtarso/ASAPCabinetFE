#include "config_ui.h"
#include "section_renderer.h"
#include "sound/isound_manager.h"
#include <set>
#include <algorithm>

ConfigUI::ConfigUI(IConfigService* configService, IKeybindProvider* keybindProvider,
                   IAssetManager* assets,[[maybe_unused]] size_t* currentIndex,[[maybe_unused]] std::vector<TableData>* tables,
                   IAppCallbacks* appCallbacks, bool& showConfig, bool standaloneMode)
    : configService_(configService),
      keybindProvider_(keybindProvider),
      assets_(assets),
      appCallbacks_(appCallbacks),
      showConfig_(showConfig),
      standaloneMode_(standaloneMode),
      standaloneFileDialog_(),
      normalFileDialog_()
{
    LOG_DEBUG("ConfigUI constructed.");
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
        LOG_ERROR("ConfigUI: Error initializing JSON data: " << e.what());
    }
    initializeRenderers();
}

void ConfigUI::initializeRenderers() {
    for (const auto& section : sectionConfig_.getSectionOrder()) {
        renderers_[section] = std::make_unique<SectionRenderer>(
            this,  // Pass the ConfigUI instance
            sectionConfig_.getKeyOrder(section));
    }
}

void ConfigUI::drawGUI() {
    ImGuiIO& io = ImGui::GetIO();
    ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | 
                                  ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse;

    if (standaloneMode_) {
        ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(io.DisplaySize.x, io.DisplaySize.y), ImGuiCond_Always);
        // ImGui::SetNextWindowFocus();
        ImGui::Begin("ASAPCabinetFE 1st Run Setup", &showConfig_, windowFlags);
    } else {
        float configUIWidth = 0.7f;  // Default if not found
        float configUIHeight = 0.5f; // Default if not found
        try {
            if (jsonData_.contains("Internal")) {
                if (jsonData_["Internal"].contains("configUIWidth")) {
                    configUIWidth = jsonData_["Internal"]["configUIWidth"].get<float>();
                }
                if (jsonData_["Internal"].contains("configUIHeight")) {
                    configUIHeight = jsonData_["Internal"]["configUIHeight"].get<float>();
                }
            }
        } catch (const std::exception& e) {
            LOG_ERROR("ConfigUI: Error accessing window size ratios: " << e.what());
        }

        float configWidth = io.DisplaySize.x * configUIWidth;
        float configHeight = io.DisplaySize.y * configUIHeight;
        float configX = io.DisplaySize.x / 2.0f - configWidth / 2.0f;
        float configY = io.DisplaySize.y / 2.0f - configHeight / 2.0f;
        
        ImGui::SetNextWindowPos(ImVec2(configX, configY), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(configWidth, configHeight), ImGuiCond_Always);
        // ImGui::SetNextWindowFocus();
        ImGui::Begin("ASAPCabinetFE Configuration", &showConfig_, windowFlags);
    }

    if (jsonData_.is_null()) {
        LOG_ERROR("ConfigUI: JSON data is null.");
        ImGui::Text("Error: Failed to load configuration data.");
        ImGui::End();
        return;
    }

    bool hasChanges = jsonData_ != originalJsonData_;

    float buttonHeight = ImGui::GetFrameHeightWithSpacing() + 15.0f;
    ImGui::BeginChild("ConfigContent", ImVec2(0, -buttonHeight), false, ImGuiWindowFlags_AlwaysVerticalScrollbar);
    std::set<std::string> renderedSections;

    // Select the appropriate file dialog instance based on mode
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
                LOG_ERROR("ConfigUI: No renderer for section VPX");
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
                LOG_ERROR("ConfigUI: No renderer for section WindowSettings");
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
                    if (sectionName == "Keybinds" && isCapturingKey_) {
                        ImGui::Text("Press a key or joystick input to bind to %s...", capturingKeyName_.c_str());
                    }
                    it->second->render(sectionName, jsonData_[sectionName], isCapturingKey_, capturingKeyName_, fileDialog, defaultOpen, isDialogOpen_, dialogKey_);
                } else {
                    LOG_ERROR("ConfigUI: No renderer for section " << sectionName);
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
            LOG_INFO("Quit without saving.");
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
        LOG_DEBUG("ConfigUI: Attempting to display dialog for key: " << dialogKey_ << ", isDialogOpen_: " << isDialogOpen_);
        ImVec2 maxSize = ImVec2(ImGui::GetIO().DisplaySize.x * 0.8f, ImGui::GetIO().DisplaySize.y * 0.8f);
        ImVec2 minSize = ImVec2(600, 400);

        if (dialogKey_ == "VPXTablesPath") {
            if (fileDialog->Display("FolderDlg_VPXTablesPath", ImGuiWindowFlags_NoCollapse, minSize, maxSize)) {
                LOG_DEBUG("ConfigUI: Displaying FolderDlg_VPXTablesPath");
                if (fileDialog->IsOk()) {
                    jsonData_["VPX"]["VPXTablesPath"] = fileDialog->GetCurrentPath();
                    LOG_INFO("ConfigUI: Selected VPXTablesPath: " << jsonData_["VPX"]["VPXTablesPath"].get<std::string>());
                }
                fileDialog->Close();
                isDialogOpen_ = false;
            }
        } else if (dialogKey_ == "VPinballXPath") {
            if (fileDialog->Display("FileDlg_VPinballXPath", ImGuiWindowFlags_NoCollapse, minSize, maxSize)) {
                LOG_DEBUG("ConfigUI: Displaying FileDlg_VPinballXPath");
                if (fileDialog->IsOk()) {
                    jsonData_["VPX"]["VPinballXPath"] = fileDialog->GetFilePathName();
                    LOG_INFO("ConfigUI: Selected VPinballXPath: " << jsonData_["VPX"]["VPinballXPath"].get<std::string>());
                }
                fileDialog->Close();
                isDialogOpen_ = false;
            }
        } else if (dialogKey_ == "vpxIniPath") {
            if (fileDialog->Display("FileDlg_vpxIniPath", ImGuiWindowFlags_NoCollapse, minSize, maxSize)) {
                LOG_DEBUG("ConfigUI: Displaying FileDlg_vpxIniPath");
                if (fileDialog->IsOk()) {
                    jsonData_["VPX"]["vpxIniPath"] = fileDialog->GetFilePathName();
                    LOG_INFO("ConfigUI: Selected vpxIniPath: " << jsonData_["VPX"]["vpxIniPath"].get<std::string>());
                }
                fileDialog->Close();
                isDialogOpen_ = false;
            }
        } else {
            LOG_ERROR("ConfigUI: Unknown dialog key: " << dialogKey_);
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
            LOG_DEBUG("ConfigUI: Captured bind: " << newBind << " for " << capturingKeyName_);
            updateKeybind(capturingKeyName_, newBind);
            isCapturingKey_ = false;
        }
    }
}

void ConfigUI::updateKeybind(const std::string& action, const std::string& bind) {
    if (!jsonData_.contains("Keybinds") || !keybindProvider_) return;

    std::string normalizedAction = action;
    for (size_t i = 0; i < normalizedAction.size(); ++i) {
        if (normalizedAction[i] == ' ') {
            normalizedAction.erase(i, 1);
            if (i < normalizedAction.size()) {
                normalizedAction[i] = std::toupper(normalizedAction[i]);
            }
        }
    }

    jsonData_["Keybinds"][action] = bind;
    SDL_Keycode key = SDL_GetKeyFromName(bind.c_str());
    if (key != SDLK_UNKNOWN) {
        keybindProvider_->setKey(normalizedAction, key);
        LOG_DEBUG("ConfigUI: Updated keybind " << normalizedAction << " to " << bind);
    } else if (bind.find("JOY_") == 0) {
        if (bind.find("_BUTTON_") != std::string::npos) {
            size_t joyEnd = bind.find("_BUTTON_");
            int joystickId = std::stoi(bind.substr(4, joyEnd - 4));
            uint8_t button = static_cast<uint8_t>(std::stoi(bind.substr(joyEnd + 8)));
            keybindProvider_->setJoystickButton(normalizedAction, joystickId, button);
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
            keybindProvider_->setJoystickHat(normalizedAction, joystickId, hat, direction);
        } else if (bind.find("_AXIS_") != std::string::npos) {
            size_t joyEnd = bind.find("_AXIS_");
            size_t axisEnd = bind.find("_", joyEnd + 6);
            int joystickId = std::stoi(bind.substr(4, joyEnd - 4));
            uint8_t axis = static_cast<uint8_t>(std::stoi(bind.substr(joyEnd + 6, axisEnd - (joyEnd + 6))));
            bool positiveDirection = (bind.substr(axisEnd + 1) == "POSITIVE");
            keybindProvider_->setJoystickAxis(normalizedAction, joystickId, axis, positiveDirection);
        }
        LOG_DEBUG("ConfigUI: Updated joystick bind " << normalizedAction << " to " << bind);
    }
}

void ConfigUI::saveConfig() {
    LOG_DEBUG("ConfigUI::saveConfig called.");
    if (!configService_ || jsonData_.is_null()) {
        LOG_ERROR("ConfigUI: Cannot save config, service or JSON data is null.");
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
                        LOG_DEBUG("ConfigUI: Detected change in " << sectionName << "." << key << ", ReloadType: " << static_cast<int>(it->second.first));
                    } else {
                        LOG_DEBUG("ConfigUI: No ReloadType found for " << sectionName << "." << key);
                    }
                }
            }
        }

        Settings& settings = const_cast<Settings&>(configService_->getSettings());
        settings = jsonData_;

        if (jsonData_.contains("Keybinds") && jsonData_["Keybinds"].is_object()) {
            for (const auto& [action, bind] : jsonData_["Keybinds"].items()) {
                std::string normalizedAction = action;
                for (size_t i = 0; i < normalizedAction.size(); ++i) {
                    if (normalizedAction[i] == ' ') {
                        normalizedAction.erase(i, 1);
                        if (i < normalizedAction.size()) {
                            normalizedAction[i] = std::toupper(normalizedAction[i]);
                        }
                    }
                }
                if (bind.is_string()) {
                    SDL_Keycode key = SDL_GetKeyFromName(bind.get<std::string>().c_str());
                    if (key != SDLK_UNKNOWN) {
                        settings.keybinds_[normalizedAction] = key;
                        LOG_DEBUG("ConfigUI: Synced keybind " << normalizedAction << " to " << bind.get<std::string>());
                    }
                }
            }
        }

        configService_->saveConfig();
        LOG_DEBUG("ConfigUI: Config saved successfully.");

        for (const auto& reloadType : reloadTypes) {
            switch (reloadType) {
                case Settings::ReloadType::None:
                    break;
                case Settings::ReloadType::Title:
                    if (assets_) {
                        assets_->setTitlePosition(settings.titleX, settings.titleY);
                        LOG_DEBUG("ConfigUI: Triggered setTitlePosition for ReloadType " << static_cast<int>(reloadType));
                    }
                    break;
                case Settings::ReloadType::Font:
                    if (appCallbacks_) {
                        appCallbacks_->reloadFont(standaloneMode_);
                        LOG_DEBUG("ConfigUI: Triggered reloadFont for ReloadType " << static_cast<int>(reloadType));
                    }
                    break;
                case Settings::ReloadType::Windows:
                    if (appCallbacks_) {
                        appCallbacks_->reloadWindows();
                        LOG_DEBUG("ConfigUI: Triggered reloadWindows");
                    }
                    break;
                case Settings::ReloadType::Assets:
                    if (appCallbacks_) {
                        appCallbacks_->reloadAssetsAndRenderers();
                        LOG_DEBUG("ConfigUI: Triggered reloadAssetsAndRenderers for ReloadType " << static_cast<int>(reloadType));
                    }
                    break;
                case Settings::ReloadType::Tables:
                    if (appCallbacks_) {
                        appCallbacks_->reloadTablesAndTitle();
                        LOG_DEBUG("ConfigUI: Triggered reloadTablesAndTitle for ReloadType " << static_cast<int>(reloadType));
                    }
                    break;
                case Settings::ReloadType::Overlay:
                    if (appCallbacks_) {
                        appCallbacks_->reloadOverlaySettings();
                        LOG_DEBUG("ConfigUI: Triggered reloadOverlaySettings");
                    }
                    break;
                case Settings::ReloadType::Audio:
                    if (appCallbacks_) {
                        ISoundManager* soundManager = appCallbacks_->getSoundManager();
                        if (soundManager) {
                            soundManager->updateSettings(configService_->getSettings());
                            LOG_DEBUG("ConfigUI: AudioSettings changed and saved, updated ISoundManager");
                        }
                        if (assets_) {
                            assets_->applyVideoAudioSettings();
                            LOG_DEBUG("ConfigUI: AudioSettings changed and saved, updated AssetManager");
                        }
                    }
                    break;
                default:
                    LOG_ERROR("ConfigUI: Unknown ReloadType " << static_cast<int>(reloadType));
                    break;
            }
        }

        originalJsonData_ = jsonData_;
    } catch (const std::exception& e) {
        LOG_ERROR("ConfigUI: Error saving config: " << e.what());
    }
}

void ConfigUI::resetSectionToDefault(const std::string& sectionName) {
    LOG_DEBUG("ConfigUI: Resetting section " << sectionName << " to default.");
    Settings defaultSettings;
    nlohmann::json defaultJson;
    to_json(defaultJson, defaultSettings);

    if (defaultJson.contains(sectionName)) {
        jsonData_[sectionName] = defaultJson[sectionName];
        LOG_DEBUG("ConfigUI: Section " << sectionName << " reset to default values.");
    } else {
        LOG_ERROR("ConfigUI: No default data found for section " << sectionName);
    }
}

void ConfigUI::refreshUIState() {
    LOG_DEBUG("ConfigUI: Refreshing UI state");
    try {
        jsonData_ = nlohmann::json(configService_->getSettings());
        originalJsonData_ = jsonData_;
        LOG_DEBUG("ConfigUI: UI state refreshed successfully");
    } catch (const std::exception& e) {
        LOG_ERROR("ConfigUI: Error refreshing UI state: " << e.what());
    }
}