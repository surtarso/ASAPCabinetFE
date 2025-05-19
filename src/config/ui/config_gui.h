#ifndef CONFIG_UI_H
#define CONFIG_UI_H

#include "config/iconfig_service.h"
#include "keybinds/ikeybind_provider.h"
#include "render/iasset_manager.h"
#include "render/table_data.h"
#include "config/ui/section_renderer.h"
#include "config/ui/button_handler.h"
#include "config/ui/input_handler.h"
#include "config/ui/config_state.h"
#include "core/iapp_callbacks.h" 
#include <string>
#include <vector>

class ConfigUI {
public:
    ConfigUI(IConfigService* configService, IKeybindProvider* keybindProvider, 
             IAssetManager* assets, size_t* currentIndex, std::vector<TableData>* tables, 
             IAppCallbacks* appCallbacks, bool& showConfig, bool standaloneMode = false);
    void drawGUI();
    void handleEvent(const SDL_Event& event);
    void saveConfig();
    bool isCapturingKey() const { return inputHandler_.isCapturingKey(); }
    bool shouldClose() const { return !showConfig_; }
    bool isStandalone() const { return standaloneMode_; }

private:
    IConfigService* configService_;
    IKeybindProvider* keybindProvider_;
    IAssetManager* assets_;
    size_t* currentIndex_;
    std::vector<TableData>* tables_;
    IAppCallbacks* appCallbacks_;
    bool& showConfig_;
    bool standaloneMode_;
    ConfigUIState state_;
    SectionRenderer sectionRenderer_;
    ButtonHandler buttonHandler_;
    InputHandler inputHandler_;

    void discardChanges();
    void renderSectionsPane();
    void renderKeyValuesPane();
    void renderButtonPane();
    void updateSaveMessageTimer();
    std::vector<std::string> getVisibleSections() const;
};

#endif // CONFIG_UI_H