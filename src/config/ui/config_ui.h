#ifndef CONFIG_UI_H
#define CONFIG_UI_H

#include "config/iconfig_service.h"
#include "keybinds/ikeybind_provider.h"
#include "render/iasset_manager.h"
#include "tables/table_data.h"
#include "core/iapp_callbacks.h"
#include "isection_renderer.h"
#include "section_config.h"
#include "ImGuiFileDialog.h"
#include <string>
#include <vector>
#include <json.hpp>
#include <memory>
#include <unordered_map>

/**
 * @class ConfigUI
 * @brief Manages the ImGui-based configuration UI.
 */
class ConfigUI {
public:
    ConfigUI(IConfigService* configService, IKeybindProvider* keybindProvider, 
             IAssetManager* assets, size_t* currentIndex, std::vector<TableData>* tables, 
             IAppCallbacks* appCallbacks, bool& showConfig, bool standaloneMode = false);

    void drawGUI();
    void handleEvent(const SDL_Event& event);
    void saveConfig();
    bool shouldClose() const { return !showConfig_; }
    bool isStandalone() const { return standaloneMode_; }
    void resetSectionToDefault(const std::string& sectionName);

private:
    IConfigService* configService_;
    IKeybindProvider* keybindProvider_;
    IAssetManager* assets_;
    IAppCallbacks* appCallbacks_;
    bool& showConfig_;
    bool standaloneMode_;
    nlohmann::json jsonData_;
    nlohmann::json originalJsonData_;
    SectionConfig sectionConfig_;
    std::unordered_map<std::string, std::unique_ptr<ISectionRenderer>> renderers_;
    std::unordered_map<std::string, bool> sectionCollapseStates_;
    // float windowWidthRatio_ = 0.8f;
    // float windowHeightRatio_ = 0.8f;

    bool isCapturingKey_ = false;
    std::string capturingKeyName_;

    bool isDialogOpen_ = false;
    std::string dialogKey_ = "";

    ImGuiFileDialog standaloneFileDialog_; // File dialog instance for standalone mode
    ImGuiFileDialog normalFileDialog_;     // File dialog instance for normal mode

    void initializeRenderers();
    void updateKeybind(const std::string& action, const std::string& bind);
};

#endif // CONFIG_UI_H