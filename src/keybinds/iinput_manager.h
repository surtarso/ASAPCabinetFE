#ifndef IINPUT_MANAGER_H
#define IINPUT_MANAGER_H

#include <SDL2/SDL.h>
#include <string>
#include <functional>

class AssetManager;
class ISoundManager;
class ConfigService;
struct TableLoader;
class ConfigUI;

class IInputManager {
public:
    virtual ~IInputManager() = default;
    virtual void handleEvent(const SDL_Event& event) = 0;
    virtual void registerActions() = 0; // Called once to set up handlers
    virtual void setDependencies(AssetManager* assets, ISoundManager* sound, ConfigService* settings,
                                size_t& currentIndex, const std::vector<TableLoader>& tables,
                                bool& showConfig, const std::string& exeDir) = 0;
    virtual void setRuntimeEditor(ConfigUI* editor) = 0;
    virtual bool isConfigActive() const = 0; // To check showConfig_ state
    virtual bool shouldQuit() const = 0;
};

#endif // IINPUT_MANAGER_H