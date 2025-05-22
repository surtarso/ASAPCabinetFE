#ifndef IINPUT_MANAGER_H
#define IINPUT_MANAGER_H

#include "render/table_data.h"
#include <SDL2/SDL.h>
#include <string>
#include <functional>
#include <vector>

// Forward declarations
class IAssetManager; 
class ISoundManager;
class IConfigService;
class ConfigUI;
class IScreenshotManager;
class IWindowManager;

/**
 * @brief Interface for managing user input and associated actions.
 *
 * The IInputManager interface provides an abstract definition for handling input events,
 * registering application-specific actions, and setting up necessary dependencies for
 * processing interactive events within the application.
 *
 * Implementers are required to define the following functionalities:
 *
 * - Processing of SDL_Event events.
 * - Registration of input-related actions.
 * - Injection of dependencies such as asset management, sound, configuration,
 *   table data, screenshot management, and window control.
 * - Integration with a runtime configuration editor.
 * - Querying the current state of the configuration mode.
 * - Determining whether the application should exit.
 */
class IInputManager {
public:
    virtual ~IInputManager() = default;
    virtual void handleEvent(const SDL_Event& event) = 0;
    virtual void registerActions() = 0;
    virtual void setDependencies(IAssetManager* assets, ISoundManager* sound, IConfigService* settings,
                                 size_t& currentIndex, const std::vector<TableData>& tables,
                                 bool& showConfig, const std::string& exeDir, IScreenshotManager* screenshotManager,
                                 IWindowManager* windowManager) = 0;
    virtual void setRuntimeEditor(ConfigUI* editor) = 0;
    virtual bool isConfigActive() const = 0;
    virtual bool shouldQuit() const = 0;
};

#endif // IINPUT_MANAGER_H