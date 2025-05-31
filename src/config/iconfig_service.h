/**
 * @file iconfig_service.h
 * @brief Defines the IConfigService interface for managing configuration in ASAPCabinetFE.
 *
 * This header provides the IConfigService interface, which specifies methods for accessing
 * and managing application settings, keybindings, and INI configuration data. It supports
 * loading, saving, and validating configurations, as well as updating window positions.
 */

#ifndef ICONFIG_SERVICE_H
#define ICONFIG_SERVICE_H

#include <filesystem>
#include "config/settings.h"
#include "keybinds/keybind_manager.h"
#include <map>
#include <string>

/**
 * @struct SettingsSection
 * @brief Structure for storing configuration section data (forward declaration).
 */
struct SettingsSection;

/**
 * @class IConfigService
 * @brief Interface for managing application configuration and keybindings.
 *
 * This pure virtual class defines methods for retrieving and modifying application
 * settings, validating configuration, loading and saving INI data, managing keybindings
 * via KeybindManager, and updating window positions for playfield, backglass, and DMD
 * displays. Implementers, such as ConfigService, handle configuration file operations.
 */
class IConfigService {
public:
    /**
     * @brief Virtual destructor for proper cleanup of derived classes.
     */
    virtual ~IConfigService() = default;

    /**
     * @brief Gets the current application settings.
     *
     * @return A const reference to the Settings object.
     */
    virtual const Settings& getSettings() const = 0;

    /**
     * @brief Checks if the configuration is valid.
     *
     * @return True if the configuration is valid, false otherwise.
     */
    virtual bool isConfigValid() const = 0;

    /**
     * @brief Loads configuration data from the INI file.
     */
    virtual void loadConfig() = 0;

    /**
     * @brief Saves configuration data to the INI file.
     *
     * @param iniData A map of section names to SettingsSection objects containing key-value pairs.
     */
    virtual void saveConfig(const std::map<std::string, SettingsSection>& iniData) = 0;

    /**
     * @brief Gets the current INI configuration data.
     *
     * @return A const reference to the map of section names to SettingsSection objects.
     */
    virtual const std::map<std::string, SettingsSection>& getIniData() const = 0;

    /**
     * @brief Sets the INI configuration data.
     *
     * @param iniData A map of section names to SettingsSection objects containing key-value pairs.
     */
    virtual void setIniData(const std::map<std::string, SettingsSection>& iniData) = 0;

    /**
     * @brief Gets the keybinding manager.
     *
     * @return A reference to the KeybindManager object.
     */
    virtual KeybindManager& getKeybindManager() = 0;

    /**
     * @brief Updates window positions in the configuration.
     *
     * @param playfieldX The x-coordinate of the playfield window.
     * @param playfieldY The y-coordinate of the playfield window.
     * @param backglassX The x-coordinate of the backglass window.
     * @param backglassY The y-coordinate of the backglass window.
     * @param dmdX The x-coordinate of the DMD window.
     * @param dmdY The y-coordinate of the DMD window.
     * @param topperX The x-coordinate of the topper window.
     * @param topperY The y-coordinate of the topper window.
     */
    virtual void updateWindowPositions(int playfieldX, int playfieldY, int backglassX, int backglassY, int dmdX, int dmdY, int topperX, int topperY) = 0;
};

#endif // ICONFIG_SERVICE_H