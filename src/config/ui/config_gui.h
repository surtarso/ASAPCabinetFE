/**
 * @file config_ui.h
 * @brief Defines the ConfigUI class for managing the configuration UI in ASAPCabinetFE.
 *
 * This header provides the ConfigUI class, which controls the ImGui-based configuration
 * UI. It integrates components like SectionRenderer, ButtonHandler, and InputHandler
 * to render configuration sections, handle user input, and manage INI data changes,
 * interacting with IConfigService, IAssetManager, and IAppCallbacks.
 */

#ifndef CONFIG_UI_H
#define CONFIG_UI_H

#include "config/iconfig_service.h"
#include "keybinds/ikeybind_provider.h"
#include "render/iasset_manager.h"
#include "tables/table_data.h"
#include "config/ui/section_renderer.h"
#include "config/ui/button_handler.h"
#include "config/ui/input_handler.h"
#include "config/ui/config_state.h"
#include "core/iapp_callbacks.h"
#include <string>
#include <vector>

/**
 * @class ConfigUI
 * @brief Manages the ImGui-based configuration UI.
 *
 * This class coordinates the rendering of configuration sections, key-value pairs,
 * and buttons, handling SDL input events and managing configuration changes. It
 * integrates with IConfigService for settings, IAssetManager for assets, and
 * IAppCallbacks for application updates, and supports standalone mode for isolated
 * configuration editing.
 */
class ConfigUI {
public:
    /**
     * @brief Constructs a ConfigUI instance.
     *
     * Initializes the configuration UI with dependencies for settings, keybindings,
     * assets, table data, application callbacks, and visibility control.
     *
     * @param configService The configuration service for accessing INI data.
     * @param keybindProvider The keybind provider for keybind definitions.
     * @param assets The asset manager for UI rendering resources.
     * @param currentIndex Pointer to the current table index.
     * @param tables Pointer to the list of table data.
     * @param appCallbacks The application callbacks for triggering updates.
     * @param showConfig Reference to the UI visibility flag.
     * @param standaloneMode True for standalone configuration mode, false otherwise.
     */
    ConfigUI(IConfigService* configService, IKeybindProvider* keybindProvider, 
             IAssetManager* assets, size_t* currentIndex, std::vector<TableData>* tables, 
             IAppCallbacks* appCallbacks, bool& showConfig, bool standaloneMode = false);

    /**
     * @brief Draws the configuration UI.
     *
     * Renders the ImGui UI, including sections, key-value pairs, and buttons, using
     * SectionRenderer and ButtonHandler.
     */
    void drawGUI();

    /**
     * @brief Handles an SDL input event.
     *
     * Processes the SDL event to capture input for keybinds or other UI interactions,
     * delegating to InputHandler.
     *
     * @param event The SDL event to process.
     */
    void handleEvent(const SDL_Event& event);

    /**
     * @brief Saves the current configuration.
     *
     * Persists the modified INI data to the configuration file via IConfigService.
     */
    void saveConfig();

    /**
     * @brief Checks if the UI is capturing a key.
     *
     * @return True if InputHandler is capturing a key, false otherwise.
     */
    bool isCapturingKey() const { return inputHandler_.isCapturingKey(); }

    /**
     * @brief Checks if the UI should close.
     *
     * @return True if the UI visibility flag is false, false otherwise.
     */
    bool shouldClose() const { return !showConfig_; }

    /**
     * @brief Checks if the UI is in standalone mode.
     *
     * @return True if in standalone mode, false otherwise.
     */
    bool isStandalone() const { return standaloneMode_; }

private:
    IConfigService* configService_;         ///< Configuration service for INI data.
    [[maybe_unused]] IKeybindProvider* keybindProvider_; ///< Keybind provider for keybind definitions.
    IAssetManager* assets_;                 ///< Asset manager for UI rendering resources.
    [[maybe_unused]] size_t* currentIndex_; ///< Pointer to the current table index.
    [[maybe_unused]] std::vector<TableData>* tables_; ///< Pointer to the list of table data.
    IAppCallbacks* appCallbacks_;           ///< Application callbacks for updates.
    bool& showConfig_;                      ///< Reference to the UI visibility flag.
    bool standaloneMode_;                   ///< Flag indicating standalone mode.
    ConfigUIState state_;                   ///< UI state for tracking changes and settings.
    SectionRenderer sectionRenderer_;       ///< Renderer for configuration sections and key-values.
    ButtonHandler buttonHandler_;           ///< Handler for UI buttons (e.g., Save, Close).
    InputHandler inputHandler_;             ///< Handler for input events (e.g., keybinds).
    bool requestFocusNextFrame_;            ///< Flag to request UI focus on the next frame.

    /**
     * @brief Discards unsaved configuration changes.
     *
     * Reverts INI data to the last saved state.
     */
    void discardChanges();

    /**
     * @brief Renders the sections selection pane.
     *
     * Displays the ImGui pane for selecting configuration sections, using SectionRenderer.
     */
    void renderSectionsPane();

    /**
     * @brief Renders the key-value editing pane.
     *
     * Displays the ImGui pane for editing key-value pairs, using SectionRenderer.
     */
    void renderKeyValuesPane();

    /**
     * @brief Renders the button pane.
     *
     * Displays the ImGui pane for buttons (e.g., Save, Close), using ButtonHandler.
     */
    void renderButtonPane();

    /**
     * @brief Updates the save message timer.
     *
     * Manages the timer for displaying save confirmation messages in the UI.
     */
    void updateSaveMessageTimer();

    /**
     * @brief Gets the list of visible configuration sections.
     *
     * @return A vector of section names to display in the UI.
     */
    std::vector<std::string> getVisibleSections() const;
};

#endif // CONFIG_UI_H