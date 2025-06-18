/**
 * @file iinput_manager.h
 * @brief Defines the IInputManager interface for handling input events in ASAPCabinetFE.
 *
 * This header provides the IInputManager interface, which specifies methods for
 * processing SDL input events, registering actions, and managing dependencies for
 * user interactions. It is implemented by classes like InputManager to coordinate
 * input handling with other application components.
 */

#ifndef IINPUT_MANAGER_H
#define IINPUT_MANAGER_H

#include "tables/table_data.h"
#include "launcher/itable_launcher.h"
#include <SDL_events.h>
#include <string>
#include <functional>
#include <vector>
#include <atomic>


/**
 * @class IAssetManager
 * @brief Interface for asset management (forward declaration).
 */
class IAssetManager;

/**
 * @class ISoundManager
 * @brief Interface for sound management (forward declaration).
 */
class ISoundManager;

/**
 * @class IConfigService
 * @brief Interface for configuration services (forward declaration).
 */
class IConfigService;

/**
 * @class ConfigUI
 * @brief Class for runtime configuration UI (forward declaration).
 */
class ConfigUI;

/**
 * @class IScreenshotManager
 * @brief Interface for screenshot management (forward declaration).
 */
class IScreenshotManager;

/**
 * @class IWindowManager
 * @brief Interface for window management (forward declaration).
 */
class IWindowManager;

/**
 * @class IInputManager
 * @brief Interface for managing user input and associated actions.
 *
 * This pure virtual class defines methods for handling SDL input events (keyboard,
 * joystick, mouse), registering application actions, and setting up dependencies
 * for interactive event processing. Implementers, such as InputManager, coordinate
 * with components like IAssetManager, ISoundManager, and IScreenshotManager to
 * manage user interactions, including table navigation, configuration UI, and
 * screenshot mode.
 */
class IInputManager {
public:
    /**
     * @brief Virtual destructor for proper cleanup of derived classes.
     */
    virtual ~IInputManager() = default;

    /**
     * @brief Handles an SDL input event.
     *
     * Processes the provided SDL event (keyboard, joystick, mouse) and triggers the
     * corresponding action based on the input configuration.
     *
     * @param event The SDL event to process.
     */
    virtual void handleEvent(const SDL_Event& event) = 0;

    /**
     * @brief Registers action handlers for input events.
     *
     * Sets up the mapping of action identifiers to their corresponding handler functions.
     */
    virtual void registerActions() = 0;

    /**
     * @brief Sets dependencies for input handling.
     *
     * Configures the input manager with references to components and data needed for
     * action handling, such as assets, sound, settings, and table data.
     *
     * @param assets The asset manager for accessing textures and video players.
     * @param sound The sound manager for playing UI sounds.
     * @param settings The configuration service for accessing settings.
     * @param currentIndex Reference to the current table index.
     * @param tables The list of table data.
     * @param showConfig Reference to the configuration UI visibility flag.
     * @param exeDir The executable directory for resolving paths.
     * @param screenshotManager The screenshot manager for screenshot mode.
     * @param windowManager The window manager for renderer access.
     */
    virtual void setDependencies(IAssetManager* assets, ISoundManager* sound, IConfigService* settings,
                                 size_t& currentIndex, const std::vector<TableData>& tables,
                                 bool& showConfig, bool& showEditor, bool& showVpsdb, const std::string& exeDir, IScreenshotManager* screenshotManager,
                                 IWindowManager* windowManager, std::atomic<bool>& isLoadingTables, ITableLauncher* tableLauncher) = 0;

    /**
     * @brief Sets the runtime configuration editor.
     *
     * Assigns the configuration UI editor for runtime settings adjustments.
     *
     * @param editor The configuration UI editor to set.
     */
    virtual void setRuntimeEditor(ConfigUI* editor) = 0;

    /**
     * @brief Checks if the configuration UI is active.
     *
     * @return True if the configuration UI is visible, false otherwise.
     */
    virtual bool isConfigActive() const = 0;

    /**
     * @brief Checks if the metadata editor UI is active.
     *
     * @return True if the editor UI is visible, false otherwise.
     */
    virtual bool isEditorActive() const = 0;

    /**
     * @brief Checks if the metadata catalog UI is active.
     *
     * @return True if the catalog UI is visible, false otherwise.
     */
    virtual bool isCatalogActive() const = 0;

    /**
     * @brief Checks if the application should quit.
     *
     * @return True if a quit action has been triggered, false otherwise.
     */
    virtual bool shouldQuit() const = 0;
};

#endif // IINPUT_MANAGER_H