#ifndef IINPUT_MANAGER_H
#define IINPUT_MANAGER_H

#include <SDL2/SDL.h>
#include <string>
#include <functional>
#include <vector>

class AssetManager;
class ISoundManager;
class IConfigService;
struct TableLoader;
class ConfigUI;
class ScreenshotManager;
class IWindowManager; // Forward declaration

class IInputManager {
public:
    virtual ~IInputManager() = default;
    virtual void handleEvent(const SDL_Event& event) = 0;
    virtual void registerActions() = 0;
    virtual void setDependencies(AssetManager* assets, ISoundManager* sound, IConfigService* settings,
                                 size_t& currentIndex, const std::vector<TableLoader>& tables,
                                 bool& showConfig, const std::string& exeDir, ScreenshotManager* screenshotManager,
                                 IWindowManager* windowManager) = 0;
    virtual void setRuntimeEditor(ConfigUI* editor) = 0;
    virtual bool isConfigActive() const = 0;
    virtual bool shouldQuit() const = 0;
};

#endif // IINPUT_MANAGER_H