/**
 * @file config_ui.h
 * @brief Defines the ConfigUI class for managing the ImGui-based configuration UI in ASAPCabinetFE.
 *
 * This header provides the ConfigUI class, which handles the rendering and management of the
 * configuration interface using ImGui. It integrates with configuration services, keybind providers,
 * asset managers, and application callbacks to provide a comprehensive UI for settings adjustment.
 */

#ifndef CONFIG_UI_H
#define CONFIG_UI_H

#include "config/iconfig_service.h"
#include "keybinds/ikeybind_provider.h"
#include "render/iasset_manager.h"
#include "tables/table_data.h"
#include "core/iapp_callbacks.h"
#include "isection_renderer.h"
#include "section_config.h"
#include "ImGuiFileDialog.h"
#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include <memory>
#include <unordered_map>

/**
 * @class ConfigUI
 * @brief Manages the ImGui-based configuration UI.
 *
 * This class provides the main interface for rendering and managing the configuration UI in
 * ASAPCabinetFE. It supports standalone and integrated modes, handles keybinding capture, file
 * dialogs, and section-specific rendering, and triggers reloads based on configuration changes.
 */
class ConfigUI {
public:
    /**
     * @brief Constructs a ConfigUI instance.
     *
     * Initializes the configuration UI with the specified dependencies and mode.
     *
     * @param configService Pointer to the IConfigService instance for configuration management.
     * @param keybindProvider Pointer to the IKeybindProvider instance for keybinding management.
     * @param assets Pointer to the IAssetManager instance for asset handling.
     * @param currentIndex Pointer to the current table index (unused in this implementation).
     * @param tables Pointer to the vector of table data (unused in this implementation).
     * @param appCallbacks Pointer to the IAppCallbacks instance for application callbacks.
     * @param showConfig Reference to a flag indicating if the UI should be shown.
     * @param standaloneMode Optional flag for standalone mode (default: false).
     */
    ConfigUI(IConfigService* configService, IKeybindProvider* keybindProvider, 
             IAssetManager* assets, size_t* currentIndex, std::vector<TableData>* tables, 
             IAppCallbacks* appCallbacks, bool& showConfig, bool standaloneMode = false);

    /**
     * @brief Draws the configuration UI.
     *
     * Renders the ImGui window with all configuration sections and buttons.
     */
    void drawGUI();

    /**
     * @brief Handles an SDL event for keybinding capture.
     *
     * Processes SDL events to capture key or joystick inputs during keybinding setup.
     *
     * @param event The SDL event to handle.
     */
    void handleEvent(const SDL_Event& event);

    /**
     * @brief Saves the current configuration.
     *
     * Applies changes to the configuration service and triggers necessary reloads.
     */
    void saveConfig();

    /**
     * @brief Checks if the UI should close.
     *
     * @return True if the UI should close, based on the showConfig_ flag.
     */
    bool shouldClose() const { return !showConfig_; }

    /**
     * @brief Checks if the UI is in standalone mode.
     *
     * @return True if the UI is in standalone mode, false otherwise.
     */
    bool isStandalone() const { return standaloneMode_; }

    /**
     * @brief Resets a section to its default values.
     *
     * Restores the specified section to its default configuration.
     *
     * @param sectionName The name of the section to reset.
     */
    void resetSectionToDefault(const std::string& sectionName);

    void refreshUIState();

private:
    IConfigService* configService_;                  ///< Pointer to the configuration service.
    IKeybindProvider* keybindProvider_;             ///< Pointer to the keybind provider.
    IAssetManager* assets_;                         ///< Pointer to the asset manager.
    IAppCallbacks* appCallbacks_;                   ///< Pointer to the application callbacks.
    bool& showConfig_;                              ///< Reference to the UI visibility flag.
    bool standaloneMode_;                           ///< Flag indicating standalone mode.
    nlohmann::json jsonData_;                       ///< Current JSON configuration data.
    nlohmann::json originalJsonData_;               ///< Original JSON configuration data for comparison.
    SectionConfig sectionConfig_;                   ///< Configuration for section rendering.
    std::unordered_map<std::string, std::unique_ptr<ISectionRenderer>> renderers_; ///< Map of section renderers.
    std::unordered_map<std::string, bool> sectionCollapseStates_; ///< Map of section collapse states.

    bool isCapturingKey_ = false;                   ///< Flag indicating if a key is being captured.
    std::string capturingKeyName_;                  ///< Name of the key being captured.

    bool isDialogOpen_ = false;                     ///< Flag indicating if a file dialog is open.
    std::string dialogKey_ = "";                    ///< Key associated with the open dialog.

    ImGuiFileDialog standaloneFileDialog_;          ///< File dialog instance for standalone mode.
    ImGuiFileDialog normalFileDialog_;              ///< File dialog instance for normal mode.

    /**
     * @brief Initializes the section renderers.
     *
     * Creates and stores renderers for each configuration section.
     */
    void initializeRenderers();

    /**
     * @brief Updates a keybinding in the configuration.
     *
     * Applies a new keybind to the specified action and synchronizes with the provider.
     *
     * @param action The action to update the keybind for.
     * @param bind The new keybind value.
     */
    void updateKeybind(const std::string& action, const std::string& bind);
};

#endif // CONFIG_UI_H