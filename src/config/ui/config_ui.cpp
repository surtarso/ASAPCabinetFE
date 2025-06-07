#include "config_ui.h"
#include "table_metadata_renderer.h"
#include "generic_section_renderer.h"
#include <set>
#include <algorithm>

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
      standaloneMode_(standaloneMode)
{
    LOG_DEBUG("ConfigUI constructed.");
    try {
        jsonData_ = nlohmann::json(configService_->getSettings());
        originalJsonData_ = jsonData_;
    } catch (const std::exception& e) {
        LOG_ERROR("ConfigUI: Error initializing JSON data: " << e.what());
    }
    initializeRenderers();
}

void ConfigUI::initializeRenderers() {
    // Unique renderers
    renderers_["TableMetadata"] = std::make_unique<TableMetadataSectionRenderer>(
        sectionConfig_.getKeyOrder("TableMetadata"));
    // Generic renderer for other sections
    for (const auto& section : sectionConfig_.getSectionOrder()) {
        if (section != "TableMetadata") {
            renderers_[section] = std::make_unique<GenericSectionRenderer>(
                sectionConfig_.getKeyOrder(section));
        }
    }
}

void ConfigUI::drawGUI() {
    ImGui::Begin("Configuration", &showConfig_, ImGuiWindowFlags_AlwaysAutoResize);

    if (jsonData_.is_null()) {
        LOG_ERROR("ConfigUI: JSON data is null.");
        ImGui::Text("Error: Failed to load configuration data.");
        ImGui::End();
        return;
    }

    bool hasChanges = jsonData_ != originalJsonData_;
    std::set<std::string> renderedSections;

    // Render sections in order
    for (const auto& sectionName : sectionConfig_.getSectionOrder()) {
        if (jsonData_.contains(sectionName)) {
            ImGui::PushID(sectionName.c_str());
            auto it = renderers_.find(sectionName);
            if (it != renderers_.end()) {
                it->second->render(sectionName, jsonData_[sectionName]);
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
        renderers_[sectionName]->render(sectionName, jsonData_[sectionName]);
        ImGui::PopID();
        ImGui::Spacing();
    }

    ImGui::Separator();
    if (hasChanges) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.8f, 0.2f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.9f, 0.3f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.1f, 0.7f, 0.1f, 1.0f));
    }
    if (ImGui::Button("Save", ImVec2(100, 0))) {
        saveConfig();
    }
    if (hasChanges) {
        ImGui::PopStyleColor(3);
    }

    ImGui::End();
}

void ConfigUI::handleEvent(const SDL_Event& event) {
    //LOG_DEBUG("ConfigUI::handleEvent called.");
    (void)event;
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
        settings = jsonData_;
        configService_->saveConfig();
        LOG_DEBUG("ConfigUI: Config saved successfully.");

        for (const auto& reloadType : reloadTypes) {
            switch (reloadType) {
                case Settings::ReloadType::None:
                    break;
                case Settings::ReloadType::Font:
                    appCallbacks_->reloadTablesAndTitle();
                    LOG_DEBUG("ConfigUI: Triggered reloadTablesAndTitle for ReloadType " << static_cast<int>(reloadType));
                    break;
                case Settings::ReloadType::Windows:
                    appCallbacks_->reloadWindows();
                    LOG_DEBUG("ConfigUI: Triggered reloadWindows");
                    break;
                case Settings::ReloadType::Assets:
                    appCallbacks_->reloadAssetsAndRenderers();
                    LOG_DEBUG("ConfigUI: Triggered reloadAssetsAndRenderers for ReloadType " << static_cast<int>(reloadType));
                    break;
                case Settings::ReloadType::Metadata:
                    appCallbacks_->reloadOverlaySettings();
                    LOG_DEBUG("ConfigUI: Triggered reloadOverlaySettings");
                    break;
                case Settings::ReloadType::Audio:
                    if (appCallbacks_->getSoundManager()) {
                        // appCallbacks_->getSoundManager()->updateSettings(jsonData_);
                        LOG_DEBUG("ConfigUI: Triggered SoundManager::updateSettings");
                    } else {
                        LOG_ERROR("ConfigUI: SoundManager is null for Audio reload");
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