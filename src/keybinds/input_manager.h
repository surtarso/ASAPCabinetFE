#ifndef INPUT_MANAGER_H
#define INPUT_MANAGER_H

#include "keybinds/iinput_manager.h"
#include "keybinds/ikeybind_provider.h"
#include "render/asset_manager.h"
#include "sound/isound_manager.h"
#include "config/settings_manager.h"
#include "render/table_loader.h"
#include "capture/screenshot_manager.h"
#include <map>
#include <vector>

class InputManager : public IInputManager {
public:
    InputManager(IKeybindProvider* keybindProvider);
    void handleEvent(const SDL_Event& event) override;
    void registerActions() override;
    void setDependencies(AssetManager* assets, ISoundManager* sound, SettingsManager* settings,
                         size_t& currentIndex, const std::vector<TableLoader>& tables,
                         bool& showConfig, const std::string& exeDir) override;
    bool isConfigActive() const override { return *showConfig_; }
    bool shouldQuit() const override { return quit_; }

private:
    using ActionHandler = std::function<void()>;

    void handleConfigEvents(const SDL_Event& event);    // Added
    void handleRegularEvents(const SDL_Event& event);   // Added

    IKeybindProvider* keybindProvider_;
    AssetManager* assets_;
    ISoundManager* soundManager_;
    SettingsManager* settingsManager_;
    size_t* currentIndex_;
    const std::vector<TableLoader>* tables_;
    bool* showConfig_;
    std::string exeDir_;
    std::map<std::string, ActionHandler> actionHandlers_;  // Declared here
    std::map<char, size_t> letterIndex_;
    bool quit_ = false;
    bool inScreenshotMode_ = false;
    std::unique_ptr<ScreenshotManager> screenshotManager_;
};

#endif // INPUT_MANAGER_H