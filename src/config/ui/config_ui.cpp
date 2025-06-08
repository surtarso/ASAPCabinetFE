#include "config_ui.h"
#include "generic_section_renderer.h"
#include "sound/isound_manager.h"
#include <set>
#include <algorithm>

ConfigUI::ConfigUI(IConfigService* configService, IKeybindProvider* keybindProvider,
                   IAssetManager* assets,[[maybe_unused]] size_t* currentIndex,[[maybe_unused]] std::vector<TableData>* tables,
                   IAppCallbacks* appCallbacks, bool& showConfig, bool standaloneMode)
    : configService_(configService),
      keybindProvider_(keybindProvider),
      assets_(assets),
    //   currentIndex_(currentIndex),
    //   tables_(tables),
      appCallbacks_(appCallbacks),
      showConfig_(showConfig),
      standaloneMode_(standaloneMode)
{
    LOG_DEBUG("ConfigUI constructed.");
    try {
        jsonData_ = nlohmann::json(configService_->getSettings());
        originalJsonData_ = jsonData_;
        // Initialize keybind values from KeybindManager
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
    } catch (const std::exception& e) {
        LOG_ERROR("ConfigUI: Error initializing JSON data: " << e.what());
    }
    initializeRenderers();
}

void ConfigUI::initializeRenderers() {
    // Generic renderer for sections
    for (const auto& section : sectionConfig_.getSectionOrder()) {
        renderers_[section] = std::make_unique<GenericSectionRenderer>(
            sectionConfig_.getKeyOrder(section));
    }
}

void ConfigUI::drawGUI() {
    ImGuiIO& io = ImGui::GetIO();
    // Calculate ConfigUI window size (add these to internal)
    float configWidth = io.DisplaySize.x * windowWidthRatio_;
    float configHeight = io.DisplaySize.y * windowHeightRatio_;
    // Center window
    float configX = io.DisplaySize.x / 2 - configWidth / 2;
    float configY = io.DisplaySize.y / 2 - configHeight / 2;
    
    ImGui::SetNextWindowPos(ImVec2(configX, configY), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(configWidth, configHeight), ImGuiCond_Always);
    
    ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | 
                                   ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse;
    ImGui::Begin("ASAPCabinetFE Configuration", &showConfig_, windowFlags);
    
    if (jsonData_.is_null()) {
        LOG_ERROR("ConfigUI: JSON data is null.");
        ImGui::Text("Error: Failed to load configuration data.");
        ImGui::End();
        return;
    }

    bool hasChanges = jsonData_ != originalJsonData_;
    // Reserve space for Apply button at bottom
    float buttonHeight = ImGui::GetFrameHeightWithSpacing() + 15.0f; // Button + padding
    ImGui::BeginChild("ConfigContent", ImVec2(0, -buttonHeight), false, ImGuiWindowFlags_AlwaysVerticalScrollbar);
    std::set<std::string> renderedSections;

    // Render sections in order
    for (const auto& sectionName : sectionConfig_.getSectionOrder()) {
        if (jsonData_.contains(sectionName)) {
            ImGui::PushID(sectionName.c_str());
            auto it = renderers_.find(sectionName);
            if (it != renderers_.end()) {
                if (sectionName == "Keybinds" && isCapturingKey_) {
                    ImGui::Text("Press a key or joystick input to bind to %s...", capturingKeyName_.c_str());
                }
                it->second->render(sectionName, jsonData_[sectionName], isCapturingKey_, capturingKeyName_);
            } else {
                LOG_ERROR("ConfigUI: No renderer for section " << sectionName);
            }
            renderedSections.insert(sectionName);
            ImGui::PopID();
            ImGui::Spacing();
        }
    }

    // Render remaining sections alphabetically
    for (const auto& [sectionName, sectionData] : jsonData_.items()) {
        if (renderedSections.count(sectionName)) continue;
        ImGui::PushID(sectionName.c_str());
        auto it = renderers_.find(sectionName);
        if (it == renderers_.end()) {
            renderers_[sectionName] = std::make_unique<GenericSectionRenderer>(
                sectionConfig_.getKeyOrder(sectionName));
        }
        if (sectionName == "Keybinds" && isCapturingKey_) {
            ImGui::Text("Press a key or joystick input to bind to %s...", capturingKeyName_.c_str());
        }
        renderers_[sectionName]->render(sectionName, jsonData_[sectionName], isCapturingKey_, capturingKeyName_);
        ImGui::PopID();
        ImGui::Spacing();
    }

    ImGui::EndChild();
    // Position button at bottom of window
    ImGui::SetCursorPosY(ImGui::GetWindowHeight() - buttonHeight);

    ImGui::Separator();
    if (hasChanges) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.8f, 0.2f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.9f, 0.3f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.1f, 0.7f, 0.1f, 1.0f));
    }
    if (ImGui::Button("Apply", ImVec2(100, 0))) {
        saveConfig();
        if(standaloneMode_){
            showConfig_ = false;
        }
    }
    if (hasChanges) {
        ImGui::PopStyleColor(3);
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

    // Normalize action name by removing spaces and converting to camelCase
    std::string normalizedAction = action;
    for (size_t i = 0; i < normalizedAction.size(); ++i) {
        if (normalizedAction[i] == ' ') {
            normalizedAction.erase(i, 1);
            if (i < normalizedAction.size()) {
                normalizedAction[i] = std::toupper(normalizedAction[i]);
            }
        }
    }

    jsonData_["Keybinds"][action] = bind; // Update JSON with new binding
    SDL_Keycode key = SDL_GetKeyFromName(bind.c_str());
    if (key != SDLK_UNKNOWN) {
        keybindProvider_->setKey(normalizedAction, key);
        LOG_DEBUG("ConfigUI: Updated keybind " << normalizedAction << " to " << bind);
    } else if (bind.find("JOY_") == 0) {
        // Parse joystick input (simplified for now, expand based on KeybindManager logic)
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
                        reloadTypes.insert(it->second);
                        LOG_DEBUG("ConfigUI: Detected change in " << sectionName << "." << key << ", ReloadType: " << static_cast<int>(it->second));
                    } else {
                        LOG_DEBUG("ConfigUI: No ReloadType found for " << sectionName << "." << key);
                    }
                }
            }
        }

        Settings& settings = const_cast<Settings&>(configService_->getSettings());
        settings = jsonData_; // This updates the entire Settings object, including keybinds

        // Explicitly sync keybinds to Settings::keybinds_ with normalized actions
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
                    appCallbacks_->reloadFont(standaloneMode_);
                    LOG_DEBUG("ConfigUI: Triggered reloadFont for ReloadType " << static_cast<int>(reloadType));
                    break;
                case Settings::ReloadType::Windows:
                    appCallbacks_->reloadWindows();
                    LOG_DEBUG("ConfigUI: Triggered reloadWindows");
                    break;
                case Settings::ReloadType::Assets:
                    appCallbacks_->reloadAssetsAndRenderers();
                    LOG_DEBUG("ConfigUI: Triggered reloadAssetsAndRenderers for ReloadType " << static_cast<int>(reloadType));
                    break;
                case Settings::ReloadType::Tables:
                    appCallbacks_->reloadTablesAndTitle();
                    LOG_DEBUG("ConfigUI: Triggered reloadTablesAndTitle for ReloadType " << static_cast<int>(reloadType));
                    break;
                case Settings::ReloadType::Overlay:
                    appCallbacks_->reloadOverlaySettings();
                    LOG_DEBUG("ConfigUI: Triggered reloadOverlaySettings");
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