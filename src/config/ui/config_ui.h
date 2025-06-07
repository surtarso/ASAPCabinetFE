#ifndef CONFIG_UI_H
#define CONFIG_UI_H

#include "config/iconfig_service.h"
#include "keybinds/ikeybind_provider.h"
#include "render/iasset_manager.h"
#include "tables/table_data.h"
#include "core/iapp_callbacks.h"
#include "section_renderer.h"
#include "section_config.h"
#include <string>
#include <vector>
#include <json.hpp>
#include <memory>
#include <unordered_map>

/**
 * @class ConfigUI
 * @brief Manages the ImGui-based configuration UI.
 *
 * This class orchestrates rendering of configuration sections using a registry of section renderers.
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

private:
    IConfigService* configService_;
    IKeybindProvider* keybindProvider_;
    IAssetManager* assets_; // for title position reload.
    size_t* currentIndex_;
    std::vector<TableData>* tables_;
    IAppCallbacks* appCallbacks_;
    bool& showConfig_;
    bool standaloneMode_;
    nlohmann::json jsonData_;
    nlohmann::json originalJsonData_;
    SectionConfig sectionConfig_;
    std::unordered_map<std::string, std::unique_ptr<ISectionRenderer>> renderers_;

    void initializeRenderers();
};

#endif // CONFIG_UI_H