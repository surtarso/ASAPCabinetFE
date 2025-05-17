#ifndef INPUT_MANAGER_H
#define INPUT_MANAGER_H

#include "keybinds/iinput_manager.h"
#include "keybinds/ikeybind_provider.h"
#include "render/iasset_manager.h"
#include "sound/isound_manager.h"
#include "config/iconfig_service.h"
#include "render/table_data.h"
#include "capture/iscreenshot_manager.h"
#include "config/ui/config_ui.h"
#include "core/iwindow_manager.h"
#include <map>
#include <vector>
#include <unordered_map>

class InputManager : public IInputManager {
public:
    InputManager(IKeybindProvider* keybindProvider);
    void handleEvent(const SDL_Event& event) override;
    void registerActions() override;
    void setDependencies(IAssetManager* assets, ISoundManager* sound, IConfigService* settings,
                         size_t& currentIndex, const std::vector<TableData>& tables,
                         bool& showConfig, const std::string& exeDir, IScreenshotManager* screenshotManager,
                         IWindowManager* windowManager) override;
    bool isConfigActive() const override { return *showConfig_; }
    bool shouldQuit() const override { return quit_; }
    void setRuntimeEditor(ConfigUI* editor) override { runtimeEditor_ = editor; }

private:
    using ActionHandler = std::function<void()>;

    void handleConfigEvents(const SDL_Event& event);
    void handleRegularEvents(const SDL_Event& event);
    void handleDoubleClick(const SDL_Event& event);

    IKeybindProvider* keybindProvider_;
    IAssetManager* assets_;
    ISoundManager* soundManager_;
    IConfigService* settingsManager_;
    IWindowManager* windowManager_;
    size_t* currentIndex_;
    const std::vector<TableData>* tables_;
    bool* showConfig_;
    std::string exeDir_;
    ConfigUI* runtimeEditor_ = nullptr;
    std::map<std::string, ActionHandler> actionHandlers_;
    std::map<char, size_t> letterIndex_;
    bool quit_ = false;
    bool inScreenshotMode_ = false;
    IScreenshotManager* screenshotManager_;
    std::unordered_map<Uint32, Uint32> lastClickTimes_; // Double-click detection
};

#endif // INPUT_MANAGER_H