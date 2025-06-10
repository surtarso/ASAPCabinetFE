/**
 * @file config_service.h
 * @brief Defines the ConfigService class for managing configuration in ASAPCabinetFE.
 *
 * This header provides the ConfigService class, which implements the IConfigService
 * interface to handle JSON-based configuration files, validate settings, manage
 * keybindings via KeybindManager, and integrate with VPinballX.ini settings. It
 * serves as the concrete implementation for configuration management in the application.
 */

#ifndef CONFIG_SERVICE_H
#define CONFIG_SERVICE_H

#include "utils/vpinballx_ini_reader.h"
#include "iconfig_service.h"
#include <nlohmann/json.hpp>
#include <string>

/**
 * @class ConfigService
 * @brief Concrete implementation of IConfigService for managing configuration.
 *
 * This class implements the IConfigService interface to load, save, and validate
 * JSON configuration data, manage keybindings, and apply VPinballX.ini settings
 * when enabled. It handles file operations, post-processing of settings, and
 * updates to window positions, providing a complete configuration solution for
 * ASAPCabinetFE.
 */
class ConfigService : public IConfigService {
public:
    /**
     * @brief Constructs a ConfigService instance.
     *
     * Initializes the configuration service with the specified configuration file path.
     *
     * @param configPath The file path to the configuration file.
     */
    ConfigService(const std::string& configPath);

    /**
     * @brief Virtual destructor for proper cleanup.
     *
     * Ensures proper cleanup of resources, though no specific cleanup is required here.
     */
    ~ConfigService() override = default;

    /**
     * @brief Gets the current application settings.
     *
     * @return A const reference to the Settings object containing current configuration.
     */
    const Settings& getSettings() const override;

    /**
     * @brief Checks if the configuration is valid.
     *
     * Validates essential parameters such as VPinballXPath and VPXTablesPath.
     *
     * @return True if the configuration is valid, false otherwise.
     */
    bool isConfigValid() const override;

    /**
     * @brief Loads configuration data from the JSON file.
     *
     * Reads and parses the configuration file, applying defaults and VPinballX.ini
     * settings if enabled.
     */
    void loadConfig() override;

    /**
     * @brief Saves the current configuration data to the JSON file.
     *
     * Writes the current settings and keybindings to the configuration file.
     */
    void saveConfig() override;

    /**
     * @brief Gets the keybinding manager.
     *
     * Provides access to the KeybindManager for managing input mappings.
     *
     * @return A reference to the KeybindManager object.
     */
    KeybindManager& getKeybindManager() override;

    /**
     * @brief Updates window setup coordinates and dimensions.
     *
     * Updates the configuration with the current window positions and sizes, then saves
     * the changes to the JSON file.
     *
     * @param playfieldX Reference to the playfield window X coordinate.
     * @param playfieldY Reference to the playfield window Y coordinate.
     * @param playfieldWidth Reference to the playfield window width.
     * @param playfieldHeight Reference to the playfield window height.
     * @param backglassX Reference to the backglass window X coordinate.
     * @param backglassY Reference to the backglass window Y coordinate.
     * @param backglassWidth Reference to the backglass window width.
     * @param backglassHeight Reference to the backglass window height.
     * @param dmdX Reference to the DMD window X coordinate.
     * @param dmdY Reference to the DMD window Y coordinate.
     * @param dmdWidth Reference to the DMD window width.
     * @param dmdHeight Reference to the DMD window height.
     * @param topperX Reference to the topper window X coordinate.
     * @param topperY Reference to the topper window Y coordinate.
     * @param topperWidth Reference to the topper window width.
     * @param topperHeight Reference to the topper window height.
     */
    void updateWindowSetup(int& playfieldX, int& playfieldY, int& playfieldWidth, int& playfieldHeight,
                          int& backglassX, int& backglassY, int& backglassWidth, int& backglassHeight,
                          int& dmdX, int& dmdY, int& dmdWidth, int& dmdHeight,
                          int& topperX, int& topperY, int& topperWidth, int& topperHeight) override;

private:
    /**
     * @brief Applies post-processing to settings.
     *
     * Adjusts settings after loading, such as resolving paths.
     */
    void applyPostProcessing();

    /**
     * @brief Applies VPinballX.ini settings to the current configuration.
     *
     * Updates settings with values from the VPinballX.ini file if available.
     *
     * @param iniSettings Optional VPinballX.ini settings to apply.
     */
    void applyIniSettings(const std::optional<VPinballXIniSettings>& iniSettings);

    /**
     * @brief Updates JSON data with VPinballX.ini values.
     *
     * Synchronizes JSON configuration with INI-derived settings.
     *
     * @param iniSettings Optional VPinballX.ini settings to apply.
     */
    void updateJsonWithIniValues(const std::optional<VPinballXIniSettings>& iniSettings);

    std::string configPath_; ///< Path to the configuration file.
    Settings settings_;      ///< Current application settings.
    KeybindManager keybindManager_; ///< Manager for keybinding configurations.
    nlohmann::json jsonData_; ///< JSON data structure for configuration storage.
};

#endif // CONFIG_SERVICE_H