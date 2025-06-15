/**
 * @file input_manager.h
 * @brief Defines the InputManager class for handling input events in ASAPCabinetFE.
 *
 * This header provides the InputManager class, which implements the IInputManager interface
 * to process SDL input events (keyboard, joystick, mouse) and trigger corresponding actions.
 * It integrates with keybind providers, asset managers, sound managers, and other components
 * to manage user interactions.
 */

#ifndef INPUT_MANAGER_H
#define INPUT_MANAGER_H

#include "keybinds/iinput_manager.h"
#include "keybinds/ikeybind_provider.h"
#include "render/iasset_manager.h"
#include "sound/isound_manager.h"
#include "config/iconfig_service.h"
#include "tables/table_data.h"
#include "capture/iscreenshot_manager.h"
#include "config/ui/config_ui.h"
#include "core/iwindow_manager.h"
#include <map>
#include <vector>
#include <unordered_map>
#include <functional>

/**
 * @class InputManager
 * @brief Processes SDL input events and triggers application actions.
 *
 * This class implements the IInputManager interface to handle keyboard, joystick, and
 * mouse events, mapping them to actions via IKeybindProvider. It coordinates with other
 * components (IAssetManager, ISoundManager, IConfigService, IScreenshotManager) to update
 * the application state, such as navigating tables, launching screenshots, or showing the
 * configuration UI.
 */
class InputManager : public IInputManager {
public:
    /**
     * @brief Constructs an InputManager instance.
     *
     * Initializes the input manager with a keybind provider for mapping inputs to actions.
     *
     * @param keybindProvider The keybind provider for input mappings.
     */
    InputManager(IKeybindProvider* keybindProvider);

    /**
     * @brief Handles an SDL input event.
     *
     * Processes the provided SDL event (keyboard, joystick, mouse) and triggers the
     * corresponding action based on the keybind configuration.
     *
     * @param event The SDL event to process.
     */
    void handleEvent(const SDL_Event& event) override;

    /**
     * @brief Registers action handlers for input events.
     *
     * Sets up the mapping of action identifiers to their corresponding handler functions.
     */
    void registerActions() override;

    /**
     * @brief Sets dependencies for input handling.
     *
     * Configures the input manager with references to other components and data needed
     * for action handling, such as assets, sound, settings, and table data.
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
    void setDependencies(IAssetManager* assets, ISoundManager* sound, IConfigService* settings,
                         size_t& currentIndex, const std::vector<TableData>& tables,
                         bool& showConfig, bool& showEditor, const std::string& exeDir, IScreenshotManager* screenshotManager,
                         IWindowManager* windowManager, std::atomic<bool>& isLoadingTables) override;

    /**
     * @brief Checks if the configuration UI is active.
     *
     * @return True if the configuration UI is visible, false otherwise.
     */
    bool isConfigActive() const override { return *showConfig_; }

    /**
     * @brief Checks if the metadata editor UI is active.
     *
     * @return True if the editor UI is visible, false otherwise.
     */
    bool isEditorActive() const override { return *showEditor_; }

    /**
     * @brief Checks if the application should quit.
     *
     * @return True if a quit action has been triggered, false otherwise.
     */
    bool shouldQuit() const override { return quit_; }

    /**
     * @brief Sets the runtime configuration editor.
     *
     * Assigns the configuration UI editor for runtime settings adjustments.
     *
     * @param editor The configuration UI editor to set.
     */
    void setRuntimeEditor(ConfigUI* editor) override { runtimeEditor_ = editor; }

private:
    /**
     * @brief Type alias for action handler functions.
     */
    using ActionHandler = std::function<void()>;

    /**
     * @brief Handles regular SDL events.
     *
     * Processes non-double-click events (keyboard, joystick, etc.) and triggers the
     * corresponding actions.
     *
     * @param event The SDL event to process.
     */
    void handleRegularEvents(const SDL_Event& event);

    /**
     * @brief Handles double-click events.
     *
     * Detects and processes double-click mouse events to trigger specific actions.
     *
     * @param event The SDL event to process.
     */
    void handleDoubleClick(const SDL_Event& event);

    IKeybindProvider* keybindProvider_; ///< Keybind provider for input mappings.
    IAssetManager* assets_;             ///< Asset manager for textures and video players.
    ISoundManager* soundManager_;       ///< Sound manager for UI sounds.
    IConfigService* settingsManager_;   ///< Configuration service for settings.
    IWindowManager* windowManager_;     ///< Window manager for renderer access.
    size_t* currentIndex_;              ///< Pointer to the current table index.
    const std::vector<TableData>* tables_; ///< Pointer to the table data list.
    bool* showConfig_;                  ///< Pointer to the configuration UI visibility flag.
    bool* showEditor_;
    std::string exeDir_;                ///< Executable directory for path resolution.
    IScreenshotManager* screenshotManager_; ///< Screenshot manager for screenshot mode.
    ConfigUI* runtimeEditor_;           ///< Configuration UI editor for runtime settings.
    std::map<std::string, ActionHandler> actionHandlers_; ///< Map of actions to handler functions.
    std::map<char, size_t> letterIndex_; ///< Map of letters to table indices for navigation.
    bool quit_;                         ///< Flag indicating if the application should quit.
    bool screenshotModeActive_;         ///< Flag indicating if screenshot mode is active.
    std::unordered_map<Uint32, Uint32> lastClickTimes_; ///< Timestamps for double-click detection.
    bool inExternalAppMode_;            ///< Flag indicating if an external application is running.
    Uint32 lastExternalAppReturnTime_;  ///< Timestamp of the last external application return.
    static const Uint32 EXTERNAL_APP_DEBOUNCE_TIME_MS = 500; ///< Debounce time (ms) after external app return.
    std::atomic<bool>* isLoadingTables_; ///< Track loading state
};

#endif // INPUT_MANAGER_H