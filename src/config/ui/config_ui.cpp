#include "config/ui/config_ui.h"
#include "core/app.h"
#include "utils/logging.h"
#include "imgui.h"

const std::vector<std::string> ConfigUI::sectionOrder_ = {
    "VPX", "DPISettings", "WindowSettings", "TitleDisplay", "CustomMedia", "MediaDimensions",  
    "Keybinds", "UISounds", "DefaultMedia", "Internal", "Table Overrides"
};

ConfigUI::ConfigUI(IConfigService* configService, IKeybindProvider* keybindProvider, 
                   IAssetManager* assets, size_t* currentIndex, std::vector<TableData>* tables, 
                   App* app, bool& showConfig, bool standaloneMode)
    : configService_(configService), 
      keybindProvider_(keybindProvider), 
      assets_(assets), 
      currentIndex_(currentIndex), 
      tables_(tables), 
      app_(app), 
      showConfig_(showConfig),
      standaloneMode_(standaloneMode),
      sectionRenderer_(configService, currentSection_, inputHandler_),
      buttonHandler_(showConfig_, saveMessageTimer_, inputHandler_),
      inputHandler_(keybindProvider) {

    // "Use" fields to silence warnings
    LOG_DEBUG("ConfigUI: Initialized with keybindProvider: " << keybindProvider_);
    LOG_DEBUG("ConfigUI: Assets: " << assets_); // maybe use for a preview
    LOG_DEBUG("ConfigUI: Current index: " << (currentIndex_ ? *currentIndex_ : 0)); // table slider idea later
    LOG_DEBUG("ConfigUI: Tables size: " << (tables_ ? tables_->size() : 0)); 

    // Initialize currentSection_ to the first visible section in sectionOrder_
    for (const auto& section : sectionOrder_) {
        if (standaloneMode_ && section != "VPX") {
            continue; // In standalone mode, only VPX is visible
        }
#ifndef DEBUG_LOGGING
        if (section == "Internal") {
            continue; // Hide Internal in release builds
        }
#endif
        // Check if the section exists in iniData before setting it
        if (configService_->getIniData().count(section) > 0) {
            currentSection_ = section;
            break;
        }
    }

    // If no valid section was found (unlikely), fall back to the first in iniData
    if (currentSection_.empty() && !configService_->getIniData().empty()) {
        currentSection_ = configService_->getIniData().begin()->first;
    }

    lastSavedIniData_ = configService_->getIniData(); // Store initial state
    buttonHandler_.setOnSave([this]() { saveConfig(); });
    buttonHandler_.setOnClose([this]() { discardChanges(); });
}

void ConfigUI::drawGUI() {
    if (!showConfig_) return;

    // Decrement the save message timer
    if (saveMessageTimer_ > 0.0f) {
        saveMessageTimer_ -= ImGui::GetIO().DeltaTime;
        if (saveMessageTimer_ < 0.0f) saveMessageTimer_ = 0.0f; // Ensure it doesn't go negative
    }

    ImGuiIO& io = ImGui::GetIO();
    if (standaloneMode_) {
        ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(io.DisplaySize.x, io.DisplaySize.y), ImGuiCond_Always);
        ImGui::Begin("ASAPCabinetFE Configuration", &showConfig_, 
                     ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar);
    } else {
        ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x / 2 - 400, io.DisplaySize.y / 2 - 250), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(800, 500), ImGuiCond_Always);
        ImGui::Begin("##ConfigUI", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar);
    }

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10, 10));
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(5, 5));
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.1f, 0.1f, 0.15f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.3f, 0.4f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.4f, 0.5f, 1.0f));

    float buttonHeight = ImGui::GetFrameHeightWithSpacing() * 1.2f;
    float paneHeight = ImGui::GetContentRegionAvail().y - buttonHeight;

    // Filter sections in standalone mode
    std::vector<std::string> visibleSections;
    for (const auto& section : sectionOrder_) {
        if (standaloneMode_ && section != "VPX") {
            continue; // In standalone mode, only show VPX
        }
    #ifndef DEBUG_LOGGING
        if (section == "Internal") {
            continue; // Hide Internal in release builds
        }
    #endif
        visibleSections.push_back(section);
    }

    ImGui::BeginChild("LeftPane", ImVec2(250, paneHeight), false);
    sectionRenderer_.renderSectionsPane(visibleSections); // Use filtered sections
    ImGui::EndChild();

    ImGui::SameLine();
    ImGui::BeginChild("RightPane", ImVec2(0, paneHeight), false);
    auto& iniData = const_cast<std::map<std::string, SettingsSection>&>(configService_->getIniData());
    sectionRenderer_.renderKeyValuesPane(iniData, hasChanges_);
    ImGui::EndChild();

    ImGui::BeginChild("ButtonPane", ImVec2(0, buttonHeight), false);
    buttonHandler_.renderButtonPane();
    
    // Clear "Saved!" message if another message needs to be shown
    if (inputHandler_.isCapturingKey()) {
        saveMessageTimer_ = 0.0f;
    }
    
    ImGui::EndChild();

    ImGui::PopStyleColor(3);
    ImGui::PopStyleVar(2);
    ImGui::End();
}

bool ConfigUI::hasWindowSettingsChanged() const {
    const auto& currentIniData = configService_->getIniData();
    auto currentIt = currentIniData.find("WindowSettings");
    auto lastIt = lastSavedIniData_.find("WindowSettings");

    if (currentIt == currentIniData.end() && lastIt != lastSavedIniData_.end()) {
        LOG_DEBUG("ConfigUI: WindowSettings section removed");
        return true;
    }
    if (currentIt != currentIniData.end() && lastIt == lastSavedIniData_.end()) {
        LOG_DEBUG("ConfigUI: WindowSettings section added");
        return true;
    }
    if (currentIt == currentIniData.end() && lastIt == lastSavedIniData_.end()) {
        LOG_DEBUG("ConfigUI: No WindowSettings section in either state");
        return false;
    }

    const auto& currentSection = currentIt->second;
    const auto& lastSection = lastIt->second;
    if (currentSection.keyValues.size() != lastSection.keyValues.size()) {
        LOG_DEBUG("ConfigUI: WindowSettings key count changed: " << currentSection.keyValues.size() << " vs " << lastSection.keyValues.size());
        return true;
    }

    for (const auto& [key, value] : currentSection.keyValues) {
        const std::string currentKey = key;
        auto lastPairIt = std::find_if(lastSection.keyValues.begin(), lastSection.keyValues.end(),
                                       [currentKey](const auto& pair) { return pair.first == currentKey; });
        if (lastPairIt == lastSection.keyValues.end()) {
            LOG_DEBUG("ConfigUI: WindowSettings new key: " << currentKey << "=" << value);
            return true;
        }
        if (lastPairIt->second != value) {
            LOG_DEBUG("ConfigUI: WindowSettings changed: " << currentKey << " from " << lastPairIt->second << " to " << value);
            return true;
        }
    }

    LOG_DEBUG("ConfigUI: No changes detected in WindowSettings");
    return false;
}

bool ConfigUI::hasVisibilitySettingsChanged() const {
    const auto& currentIniData = configService_->getIniData();
    auto currentIt = currentIniData.find("WindowSettings");
    auto lastIt = lastSavedIniData_.find("WindowSettings");

    // Case 1: Section removed, no need to reload assets
    if (currentIt == currentIniData.end() && lastIt != lastSavedIniData_.end()) {
        LOG_DEBUG("ConfigUI: WindowSettings section removed, no visibility settings to enable");
        return false;
    }

    // Case 2: Section added, check if ShowDMD/ShowBackglass is true
    if (currentIt != currentIniData.end() && lastIt == lastSavedIniData_.end()) {
        const auto& currentSection = currentIt->second;
        for (const auto& [key, value] : currentSection.keyValues) {
            if ((key == "ShowDMD" || key == "ShowBackglass") && value == "true") {
                LOG_DEBUG("ConfigUI: Visibility setting added: " << key << "=true");
                return true;
            }
        }
        LOG_DEBUG("ConfigUI: WindowSettings section added, no visibility settings enabled");
        return false;
    }

    // Case 3: No WindowSettings section in either state
    if (currentIt == currentIniData.end() && lastIt == lastSavedIniData_.end()) {
        LOG_DEBUG("ConfigUI: No WindowSettings section in either state");
        return false;
    }

    // Case 4: Section exists in both, check for changes to true
    const auto& currentSection = currentIt->second;
    const auto& lastSection = lastIt->second;

    for (const auto& [key, value] : currentSection.keyValues) {
        if (key == "ShowDMD" || key == "ShowBackglass") {
            const std::string currentKey = key;
            auto lastPairIt = std::find_if(lastSection.keyValues.begin(), lastSection.keyValues.end(),
                                           [currentKey](const auto& pair) { return pair.first == currentKey; });
            if (lastPairIt == lastSection.keyValues.end()) {
                if (value == "true") {
                    LOG_DEBUG("ConfigUI: Visibility setting added: " << currentKey << "=true");
                    return true;
                }
            } else if (lastPairIt->second != value && value == "true") {
                LOG_DEBUG("ConfigUI: Visibility setting changed: " << currentKey << " from " << lastPairIt->second << " to true");
                return true;
            }
        }
    }

    LOG_DEBUG("ConfigUI: No visibility settings changed to true");
    return false;
}

bool ConfigUI::hasFontSettingsChanged() const {
    const auto& currentIniData = configService_->getIniData();
    auto currentIt = currentIniData.find("TitleDisplay");
    auto lastIt = lastSavedIniData_.find("TitleDisplay");

    if (currentIt == currentIniData.end() && lastIt != lastSavedIniData_.end()) {
        LOG_DEBUG("ConfigUI: TitleDisplay section removed");
        return true;
    }
    if (currentIt != currentIniData.end() && lastIt == lastSavedIniData_.end()) {
        LOG_DEBUG("ConfigUI: TitleDisplay section added");
        return true;
    }
    if (currentIt == currentIniData.end() && lastIt == lastSavedIniData_.end()) {
        LOG_DEBUG("ConfigUI: No TitleDisplay section in either state");
        return false;
    }

    const auto& currentSection = currentIt->second;
    const auto& lastSection = lastIt->second;
    if (currentSection.keyValues.size() != lastSection.keyValues.size()) {
        LOG_DEBUG("ConfigUI: TitleDisplay key count changed: " << currentSection.keyValues.size() << " vs " << lastSection.keyValues.size());
        return true;
    }

    for (const auto& [key, value] : currentSection.keyValues) {
        if (key == "FontSize" || key == "FontPath" || key == "FontColor") {
            const std::string currentKey = key;
            auto lastPairIt = std::find_if(lastSection.keyValues.begin(), lastSection.keyValues.end(),
                                           [currentKey](const auto& pair) { return pair.first == currentKey; });
            if (lastPairIt == lastSection.keyValues.end()) {
                LOG_DEBUG("ConfigUI: Font setting added: " << currentKey << "=" << value);
                return true;
            }
            if (lastPairIt->second != value) {
                LOG_DEBUG("ConfigUI: Font setting changed: " << currentKey << " from " << lastPairIt->second << " to " << value);
                return true;
            }
        }
    }

    LOG_DEBUG("ConfigUI: No font settings changed");
    return false;
}

bool ConfigUI::hasTitleDataSourceChanged() const {
    const auto& currentIniData = configService_->getIniData();
    auto currentIt = currentIniData.find("TitleDisplay");
    auto lastIt = lastSavedIniData_.find("TitleDisplay");

    if (currentIt == currentIniData.end() || lastIt == lastSavedIniData_.end()) {
        LOG_DEBUG("ConfigUI: TitleDisplay section missing in current or last state");
        return true;
    }

    const auto& currentSection = currentIt->second;
    const auto& lastSection = lastIt->second;
    const std::string key = "TitleSource";
    auto currentPairIt = std::find_if(currentSection.keyValues.begin(), currentSection.keyValues.end(),
                                      [key](const auto& pair) { return pair.first == key; });
    auto lastPairIt = std::find_if(lastSection.keyValues.begin(), lastSection.keyValues.end(),
                                   [key](const auto& pair) { return pair.first == key; });

    if (currentPairIt == currentSection.keyValues.end() || lastPairIt == lastSection.keyValues.end()) {
        LOG_DEBUG("ConfigUI: TitleSource key missing in current or last state");
        return true;
    }
    if (currentPairIt->second != lastPairIt->second) {
        LOG_DEBUG("ConfigUI: TitleSource changed from " << lastPairIt->second << " to " << currentPairIt->second);
        return true;
    }

    LOG_DEBUG("ConfigUI: No TitleSource changes");
    return false;
}

void ConfigUI::saveConfig() {
    if (!hasChanges_) {
        LOG_DEBUG("ConfigUI: 'ConfigUI::saveConfig' called, but no changes detected, skipping save");
        return;
    }

    LOG_DEBUG("ConfigUI: 'ConfigUI::saveConfig' called");
    configService_->saveConfig(configService_->getIniData());
    bool windowSettingsChanged = hasWindowSettingsChanged();
    bool visibilitySettingsChanged = hasVisibilitySettingsChanged();
    bool fontSettingsChanged = hasFontSettingsChanged();
    bool titleDataSourceChanged = hasTitleDataSourceChanged();

    lastSavedIniData_ = configService_->getIniData();

    // Update title position if title texture exists
    if (assets_ && assets_->getTitleTexture()) {
        const Settings& settings = configService_->getSettings();
        assets_->setTitlePosition(settings.titleX, settings.titleY);
        LOG_DEBUG("ConfigUI: Updated title position to x=" << settings.titleX << ", y=" << settings.titleY);
    }

    if (app_ && !standaloneMode_) {
        if (fontSettingsChanged) {
            LOG_DEBUG("ConfigUI: Font settings changed, reloading font");
            app_->reloadFont(standaloneMode_);
        }
        if (windowSettingsChanged) {
            LOG_DEBUG("ConfigUI: WindowSettings changed, triggering window reload");
            app_->reloadWindows();
        }
        if (visibilitySettingsChanged) {
            LOG_DEBUG("ConfigUI: ShowDMD or ShowBackglass changed, triggering asset reload");
            app_->reloadAssetsAndRenderers();
        }
        if (titleDataSourceChanged) {
            LOG_DEBUG("ConfigUI: Title data-source changed, triggering table reload");
            app_->reloadTablesAndTitle();
        }
    }

    hasChanges_ = false;
    saveMessageTimer_ = 1.5f;
}

void ConfigUI::discardChanges() {
    LOG_DEBUG("ConfigUI: 'ConfigUI::discardChanges' called");
    configService_->setIniData(lastSavedIniData_);
    saveMessageTimer_ = 0.0f; // Reset timer when discarding changes
    hasChanges_ = false;
}

void ConfigUI::handleEvent(const SDL_Event& event) {
    auto& iniData = const_cast<std::map<std::string, SettingsSection>&>(configService_->getIniData());
    inputHandler_.handleEvent(event, iniData, currentSection_);
}