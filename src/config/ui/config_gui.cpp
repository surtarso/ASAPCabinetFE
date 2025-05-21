#include "config/ui/config_gui.h"
#include "core/playfield_overlay.h"
#include "utils/logging.h"
#include "imgui.h"

ConfigUI::ConfigUI(IConfigService* configService, IKeybindProvider* keybindProvider, 
                   IAssetManager* assets, size_t* currentIndex, std::vector<TableData>* tables, 
                   IAppCallbacks* appCallbacks, bool& showConfig, bool standaloneMode)
    : configService_(configService), 
      keybindProvider_(keybindProvider), 
      assets_(assets), 
      currentIndex_(currentIndex), 
      tables_(tables), 
      appCallbacks_(appCallbacks), 
      showConfig_(showConfig),
      standaloneMode_(standaloneMode),
      state_(configService),  // Initialize state with config service
      sectionRenderer_(configService, state_.currentSection, inputHandler_),
      buttonHandler_(showConfig_, state_.saveMessageTimer, inputHandler_),
      inputHandler_(keybindProvider) {

    // Set up event handlers
    buttonHandler_.setOnSave([this]() { saveConfig(); });
    buttonHandler_.setOnClose([this]() { discardChanges(); });
}

void ConfigUI::drawGUI() {
    if (!showConfig_) return;
    updateSaveMessageTimer();

    ImGuiIO& io = ImGui::GetIO();

    if (requestFocusNextFrame_) {
        ImGui::SetNextWindowFocus();
        requestFocusNextFrame_ = false; // Reset the flag after use
        LOG_DEBUG("ConfigUI: Applying ImGui::SetNextWindowFocus for config window.");
    }

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

    renderSectionsPane();
    ImGui::SameLine();
    renderKeyValuesPane();
    renderButtonPane();

    if (inputHandler_.isCapturingKey()) {
        state_.saveMessageTimer = 0.0f;
    }

    ImGui::PopStyleColor(3);
    ImGui::PopStyleVar(2);
    ImGui::End();
}

void ConfigUI::renderSectionsPane() {
    ImGui::BeginChild("LeftPane", ImVec2(250, ImGui::GetContentRegionAvail().y - ImGui::GetFrameHeightWithSpacing() * 1.2f), false);
    sectionRenderer_.renderSectionsPane(getVisibleSections());
    ImGui::EndChild();
}

void ConfigUI::renderKeyValuesPane() {
    ImGui::BeginChild("RightPane", ImVec2(0, ImGui::GetContentRegionAvail().y - ImGui::GetFrameHeightWithSpacing() * 1.2f), false);
    auto& iniData = const_cast<std::map<std::string, SettingsSection>&>(configService_->getIniData());
    sectionRenderer_.renderKeyValuesPane(iniData, state_.hasChanges);
    ImGui::EndChild();
}

void ConfigUI::renderButtonPane() {
    ImGui::BeginChild("ButtonPane", ImVec2(0, ImGui::GetFrameHeightWithSpacing() * 1.2f), false);
    buttonHandler_.renderButtonPane();
    ImGui::EndChild();
}

void ConfigUI::updateSaveMessageTimer() {
    if (state_.saveMessageTimer > 0.0f) {
        state_.saveMessageTimer -= ImGui::GetIO().DeltaTime;
        if (state_.saveMessageTimer < 0.0f) state_.saveMessageTimer = 0.0f;
    }
}

std::vector<std::string> ConfigUI::getVisibleSections() const {
    static const std::vector<std::string> sectionOrder = {
        "VPX", "DPISettings", "WindowSettings", "TitleDisplay", "CustomMedia", "MediaDimensions",  
        "Keybinds", "UISounds", "DefaultMedia", "Internal", "Table Overrides"
    };
    std::vector<std::string> visibleSections;
    for (const auto& section : sectionOrder) {
        if (standaloneMode_ && section != "VPX") continue;
#ifndef DEBUG_LOGGING
        if (section == "Internal" || section == "UISounds" || section == "DefaultMedia") continue;
#endif
        visibleSections.push_back(section);
    }
    return visibleSections;
}

void ConfigUI::saveConfig() {
    if (!state_.hasChanges) {
        LOG_DEBUG("ConfigUI: 'ConfigUI::saveConfig' called, but no changes detected, skipping save");
        return;
    }

    LOG_DEBUG("ConfigUI: 'ConfigUI::saveConfig' started");

    // Store current iniData before saving
    const auto oldIniData = configService_->getIniData();
    
    // Save the config
    configService_->saveConfig(configService_->getIniData());
    const auto& currentIniData = configService_->getIniData();
    
    // Check for changes using oldIniData
    bool windowSettingsChanged = state_.hasWindowSettingsChanged(oldIniData);
    bool visibilitySettingsChanged = state_.hasVisibilitySettingsChanged(oldIniData);
    bool fontSettingsChanged = state_.hasFontSettingsChanged(oldIniData);
    bool titleDataSourceChanged = state_.hasTitleDataSourceChanged(oldIniData);
    bool videoBackendChanged = state_.hasVideoBackendChanged(oldIniData);
    bool forceImagesOnlyChanged = state_.hasForceImagesOnlyChanged(oldIniData);
    bool metadataSettingsChanged = state_.hasMetadataSettingsChanged(oldIniData);

    // Log change detection results
    LOG_DEBUG("ConfigUI: Change detection: "
              << "windowSettings=" << windowSettingsChanged
              << ", visibility=" << visibilitySettingsChanged
              << ", font=" << fontSettingsChanged
              << ", titleDataSource=" << titleDataSourceChanged
              << ", videoBackend=" << videoBackendChanged
              << ", forceImagesOnly=" << forceImagesOnlyChanged
              << ", metadata=" << metadataSettingsChanged);

    // Update lastSavedIniData
    state_.lastSavedIniData = currentIniData;

    // Update title position if needed
    if (assets_ && assets_->getTitleTexture()) {
        const Settings& settings = configService_->getSettings();
        assets_->setTitlePosition(settings.titleX, settings.titleY);
        LOG_DEBUG("ConfigUI: Updated title position to x=" << settings.titleX << ", y=" << settings.titleY);
    }

    // Trigger callbacks for relevant changes
    if (appCallbacks_ && !standaloneMode_) {
        bool anyCallbackTriggered = false;
        if (fontSettingsChanged) {
            LOG_DEBUG("ConfigUI: Font settings changed, triggering reloadFont");
            appCallbacks_->reloadFont(standaloneMode_);
            anyCallbackTriggered = true;
        }
        if (windowSettingsChanged) {
            LOG_DEBUG("ConfigUI: WindowSettings changed, triggering reloadWindows");
            appCallbacks_->reloadWindows();
            anyCallbackTriggered = true;
        }
        if (visibilitySettingsChanged || videoBackendChanged) {
            LOG_DEBUG("ConfigUI: ShowDMD, ShowBackglass, or videoBackend changed, triggering reloadAssetsAndRenderers");
            appCallbacks_->reloadAssetsAndRenderers();
            anyCallbackTriggered = true;
        }
        if (titleDataSourceChanged || forceImagesOnlyChanged) {
            LOG_DEBUG("ConfigUI: Title data-source or ForceImagesOnly changed, triggering reloadTablesAndTitle");
            appCallbacks_->reloadTablesAndTitle();
            anyCallbackTriggered = true;
        }
        if (metadataSettingsChanged) {
            LOG_DEBUG("ConfigUI: ShowMetadata changed, triggering reloadOverlaySettings");
            appCallbacks_->reloadOverlaySettings();
            anyCallbackTriggered = true;
        }
        if (!anyCallbackTriggered) {
            LOG_DEBUG("ConfigUI: No callbacks triggered");
        }
    } else {
        LOG_DEBUG("ConfigUI: Callbacks skipped (appCallbacks_ is null or standaloneMode_)");
    }

    state_.hasChanges = false;
    state_.saveMessageTimer = 1.5f;
    LOG_DEBUG("ConfigUI: Save completed");

    requestFocusNextFrame_ = true;
}

void ConfigUI::discardChanges() {
    LOG_DEBUG("ConfigUI: 'ConfigUI::discardChanges' called");
    // Reload config from file instead of using lastSavedIniData
    if (state_.hasChanges == true) {
        configService_->loadConfig();
        state_.saveMessageTimer = 0.0f;
        state_.hasChanges = false;
    }
    showConfig_ = false; // Ensure UI closes
}

void ConfigUI::handleEvent(const SDL_Event& event) {
    auto& iniData = const_cast<std::map<std::string, SettingsSection>&>(configService_->getIniData());
    inputHandler_.handleEvent(event, iniData, state_.currentSection);
}