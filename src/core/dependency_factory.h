#ifndef DEPENDENCY_FACTORY_H
#define DEPENDENCY_FACTORY_H

#include <memory>
#include <string>
#include "core/iwindow_manager.h"
#include "core/gui_manager.h"
#include "render/iasset_manager.h"
#include "render/table_data.h"
#include "render/renderer.h"
#include "keybinds/input_manager.h"
#include "config/iconfig_service.h"
#include "config/ui/config_gui.h"
#include "capture/iscreenshot_manager.h"
#include "sound/isound_manager.h"

class App;

class DependencyFactory {
public:
    static std::unique_ptr<IWindowManager> createWindowManager(const Settings& settings);
    static std::unique_ptr<GuiManager> createGuiManager(IWindowManager* windowManager, IConfigService* configService);
    static std::unique_ptr<IAssetManager> createAssetManager(IWindowManager* windowManager, TTF_Font* font, 
                                                            IConfigService* configService, size_t index, 
                                                            const std::vector<TableData>& tables);
    static std::unique_ptr<Renderer> createRenderer(IWindowManager* windowManager);
    static std::unique_ptr<ISoundManager> createSoundManager(const std::string& exeDir, const Settings& settings);
    static std::unique_ptr<ConfigService> createConfigService(const std::string& configPath);
    static std::unique_ptr<IScreenshotManager> createScreenshotManager(const std::string& exeDir, 
                                                                      IConfigService* configService, 
                                                                      ISoundManager* soundManager);
    static std::unique_ptr<InputManager> createInputManager(IConfigService* configService, 
                                                           IScreenshotManager* screenshotManager);
    static std::unique_ptr<ConfigUI> createConfigUI(IConfigService* configService, IAssetManager* assets, 
                                                    size_t* currentIndex, std::vector<TableData>* tables, 
                                                    App* app, bool& showConfig);
};

#endif // DEPENDENCY_FACTORY_H