#ifndef INPUT_MANAGER_H
#define INPUT_MANAGER_H

#include "keybinds/iinput_manager.h"
#include "keybinds/ikeybind_provider.h"
#include "render/asset_manager.h"
#include "sound/isound_manager.h"
#include "config/iconfig_service.h"
#include "render/table_loader.h"
#include "capture/screenshot_manager.h"
#include "config/ui/config_ui.h"
#include <map>
#include <vector>

class InputManager : public IInputManager {
public:
    InputManager(IKeybindProvider* keybindProvider);
    void handleEvent(const SDL_Event& event) override;
    void registerActions() override;
    void setDependencies(AssetManager* assets, ISoundManager* sound, IConfigService* settings,
                         size_t& currentIndex, const std::vector<TableLoader>& tables,
                         bool& showConfig, const std::string& exeDir, ScreenshotManager* screenshotManager) override;  // Added ScreenshotManager*
    bool isConfigActive() const override { return *showConfig_; }
    bool shouldQuit() const override { return quit_; }
    void setRuntimeEditor(ConfigUI* editor) override { runtimeEditor_ = editor; }

private:
    using ActionHandler = std::function<void()>;

    void handleConfigEvents(const SDL_Event& event);
    void handleRegularEvents(const SDL_Event& event);

    IKeybindProvider* keybindProvider_;
    AssetManager* assets_;
    ISoundManager* soundManager_;
    IConfigService* settingsManager_;
    size_t* currentIndex_;
    const std::vector<TableLoader>* tables_;
    bool* showConfig_;
    std::string exeDir_;
    ConfigUI* runtimeEditor_ = nullptr;
    std::map<std::string, ActionHandler> actionHandlers_;
    std::map<char, size_t> letterIndex_;
    bool quit_ = false;
    bool inScreenshotMode_ = false;
    ScreenshotManager* screenshotManager_;  // Changed to raw pointer, injected
};

#endif // INPUT_MANAGER_H