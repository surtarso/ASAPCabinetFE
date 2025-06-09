/**
 * @file iconfig_service.h
 * @brief Defines the IConfigService interface for managing configuration in ASAPCabinetFE.
 *
 * This header provides the IConfigService interface, which specifies methods for
 * retrieving and modifying application settings, validating configuration, loading
 * and saving JSON-based configuration data, managing keybindings, and updating
 * window positions. It is implemented by classes like ConfigService to handle
 * configuration file operations and provide a consistent API for the application.
 */

#ifndef ICONFIG_SERVICE_H
#define ICONFIG_SERVICE_H

#include <filesystem>
#include "settings.h"
#include "keybinds/keybind_manager.h"
#include <string>

/**
 * @class IConfigService
 * @brief Interface for managing application configuration and keybindings in ASAPCabinetFE.
 *
 * This pure virtual class defines methods for retrieving and modifying application
 * settings, validating configuration, loading and saving JSON-based configuration data,
 * managing keybindings via KeybindManager, and updating window positions for playfield,
 * backglass, DMD, and topper displays. Implementers handle JSON configuration file operations
 * and provide a standardized interface for configuration management.
 */
class IConfigService {
public:
    /**
     * @brief Virtual destructor for proper cleanup of derived classes.
     *
     * Ensures proper cleanup of any derived classes implementing this interface.
     */
    virtual ~IConfigService() = default;

    /**
     * @brief Gets the current application settings.
     *
     * @return A const reference to the Settings object containing current configuration.
     */
    virtual const Settings& getSettings() const = 0;

    /**
     * @brief Gets the keybinding manager.
     *
     * Provides access to the KeybindManager for managing input mappings.
     *
     * @return A reference to the KeybindManager object.
     */
    virtual KeybindManager& getKeybindManager() = 0;

    /**
     * @brief Checks if the configuration is valid.
     *
     * Validates essential configuration parameters such as file paths and permissions.
     *
     * @return True if the configuration is valid, false otherwise.
     */
    virtual bool isConfigValid() const = 0;

    /**
     * @brief Loads configuration data from the JSON file.
     *
     * Reads and parses the configuration file, applying defaults if the file is missing.
     */
    virtual void loadConfig() = 0;

    /**
     * @brief Saves the current configuration data to the JSON file.
     *
     * Writes the current settings and keybindings to the configuration file.
     */
    virtual void saveConfig() = 0;

    /**
     * @brief Updates window setup coordinates and dimensions.
     *
     * Updates the configuration with the current window positions and sizes for
     * playfield, backglass, DMD, and topper displays, then saves the changes.
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
    virtual void updateWindowSetup(int& playfieldX, int& playfieldY, int& playfieldWidth, int& playfieldHeight,
                                  int& backglassX, int& backglassY, int& backglassWidth, int& backglassHeight,
                                  int& dmdX, int& dmdY, int& dmdWidth, int& dmdHeight,
                                  int& topperX, int& topperY, int& topperWidth, int& topperHeight) = 0;
};

#endif // ICONFIG_SERVICE_H