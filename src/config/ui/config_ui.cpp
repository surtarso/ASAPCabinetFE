#include "config/ui/config_ui.h"
#include "core/app.h"
#include "utils/logging.h"
#include "imgui.h"

const std::vector<std::string> ConfigUI::sectionOrder_ = {
    "VPX", "DPISettings", "WindowSettings", "TitleDisplay", "CustomMedia", "MediaDimensions",  
    "Keybinds", "UISounds", "DefaultMedia", "Internal", "Table Overrides"
};

ConfigUI::ConfigUI(IConfigService* configService, IKeybindProvider* keybindProvider, 
                   AssetManager* assets, size_t* currentIndex, std::vector<TableLoader>* tables, 
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
      buttonHandler_(configService, app, showConfig_, hasChanges_, saveMessageTimer_, inputHandler_, standaloneMode),
      inputHandler_(keybindProvider) {
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
    sectionRenderer_.renderKeyValuesPane(iniData);
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

void ConfigUI::saveConfig() {
    LOG_DEBUG("ConfigUI::saveConfig called");
    configService_->saveConfig(configService_->getIniData());
    lastSavedIniData_ = configService_->getIniData(); // Update last saved state
    if (app_) app_->onConfigSaved(standaloneMode_);
    hasChanges_ = false;
    saveMessageTimer_ = 1.5f; // Reduced from 3.0f to 1.5f
}

void ConfigUI::discardChanges() {
    LOG_DEBUG("ConfigUI::discardChanges called");
    configService_->setIniData(lastSavedIniData_);
    saveMessageTimer_ = 0.0f; // Reset timer when discarding changes
    hasChanges_ = false;
}

void ConfigUI::handleEvent(const SDL_Event& event) {
    auto& iniData = const_cast<std::map<std::string, SettingsSection>&>(configService_->getIniData());
    inputHandler_.handleEvent(event, iniData, currentSection_);
}
