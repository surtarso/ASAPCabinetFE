#ifndef CONFIG_UI_H
#define CONFIG_UI_H

#include "config/iconfig_service.h"
#include "keybinds/ikeybind_provider.h"
#include "render/iasset_manager.h"
#include "render/table_data.h"
#include "config/ui/section_renderer.h"
#include "config/ui/button_handler.h"
#include "config/ui/input_handler.h"
#include <string>
#include <vector>
#include <map>

class App;

class ConfigUI {
public:
    ConfigUI(IConfigService* configService, IKeybindProvider* keybindProvider, 
             IAssetManager* assets, size_t* currentIndex, std::vector<TableData>* tables, 
             App* app, bool& showConfig, bool standaloneMode = false);
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
    App* app_;
    bool& showConfig_;
    bool standaloneMode_;
    std::string currentSection_;
    bool hasChanges_ = false;
    float saveMessageTimer_ = 0.0f;
    std::map<std::string, bool> showPicker_;
    std::map<std::string, SettingsSection> lastSavedIniData_;

    SectionRenderer sectionRenderer_;
    ButtonHandler buttonHandler_;
    InputHandler inputHandler_;

    static const std::vector<std::string> sectionOrder_;
    void discardChanges();
    bool hasWindowSettingsChanged() const;
    bool hasVisibilitySettingsChanged() const;
    bool hasFontSettingsChanged() const;
};

#endif // CONFIG_UI_H