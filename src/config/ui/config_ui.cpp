#include "config/ui/config_ui.h"
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
    LOG_DEBUG("ConfigUI: Initialized with keybindProvider: " << keybindProvider_);
    LOG_DEBUG("ConfigUI: Assets: " << assets_);
    LOG_DEBUG("ConfigUI: Current index: " << (currentIndex_ ? *currentIndex_ : 0));
    LOG_DEBUG("ConfigUI: Tables size: " << (tables_ ? tables_->size() : 0));

    // Set up event handlers
    buttonHandler_.setOnSave([this]() { saveConfig(); });
    buttonHandler_.setOnClose([this]() { discardChanges(); });
}

void ConfigUI::drawGUI() {
    if (!showConfig_) return;
    updateSaveMessageTimer();

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
        if (section == "Internal") continue;
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

    LOG_DEBUG("ConfigUI: 'ConfigUI::saveConfig' called");
    configService_->saveConfig(configService_->getIniData());
    const auto& currentIniData = configService_->getIniData();
    bool windowSettingsChanged = state_.hasWindowSettingsChanged(currentIniData);
    bool visibilitySettingsChanged = state_.hasVisibilitySettingsChanged(currentIniData);
    bool fontSettingsChanged = state_.hasFontSettingsChanged(currentIniData);
    bool titleDataSourceChanged = state_.hasTitleDataSourceChanged(currentIniData);

    state_.lastSavedIniData = currentIniData;

    if (assets_ && assets_->getTitleTexture()) {
        const Settings& settings = configService_->getSettings();
        assets_->setTitlePosition(settings.titleX, settings.titleY);
        LOG_DEBUG("ConfigUI: Updated title position to x=" << settings.titleX << ", y=" << settings.titleY);
    }

    if (appCallbacks_ && !standaloneMode_) {
        if (fontSettingsChanged) {
            LOG_DEBUG("ConfigUI: Font settings changed, reloading font");
            appCallbacks_->reloadFont(standaloneMode_);
        }
        if (windowSettingsChanged) {
            LOG_DEBUG("ConfigUI: WindowSettings changed, triggering window reload");
            appCallbacks_->reloadWindows();
        }
        if (visibilitySettingsChanged) {
            LOG_DEBUG("ConfigUI: ShowDMD or ShowBackglass changed, triggering asset reload");
            appCallbacks_->reloadAssetsAndRenderers();
        }
        if (titleDataSourceChanged) {
            LOG_DEBUG("ConfigUI: Title data-source changed, triggering table reload");
            appCallbacks_->reloadTablesAndTitle();
        }
    }

    state_.hasChanges = false;
    state_.saveMessageTimer = 1.5f;
}

void ConfigUI::discardChanges() {
    LOG_DEBUG("ConfigUI: 'ConfigUI::discardChanges' called");
    configService_->setIniData(state_.lastSavedIniData);
    state_.saveMessageTimer = 0.0f;
    state_.hasChanges = false;
}

void ConfigUI::handleEvent(const SDL_Event& event) {
    auto& iniData = const_cast<std::map<std::string, SettingsSection>&>(configService_->getIniData());
    inputHandler_.handleEvent(event, iniData, state_.currentSection);
}