/**
 * @file config_service.h
 * @brief Defines the ConfigService class for managing configuration in ASAPCabinetFE.
 *
 * This header provides the ConfigService class, which implements the IConfigService
 * interface to manage application settings, keybindings, and INI configuration data.
 * It uses SettingsParser, ConfigFileHandler, and DefaultConfigFactory to load, save,
 * and validate configurations.
 */

#ifndef CONFIG_SERVICE_H
#define CONFIG_SERVICE_H

#include "config/iconfig_service.h"
#include "config/settings.h"
#include "config/config_file_handler.h"
#include "config/settings_section.h"
#include "config/settings_parser.h"
#include "config/default_config_factory.h"
#include "keybinds/keybind_manager.h"
#include <map>
#include <string>
#include <vector>

/**
 * @class ConfigService
 * @brief Manages application configuration and keybindings.
 *
 * This class implements the IConfigService interface to handle loading, saving, and
 * validating INI configuration data, managing settings via a Settings object, and
 * configuring keybindings via a KeybindManager. It integrates with ConfigFileHandler
 * for file operations, SettingsParser for parsing, and DefaultConfigFactory for default values.
 */
class ConfigService : public IConfigService {
public:
    /**
     * @brief Constructs a ConfigService instance.
     *
     * Initializes the configuration service with the specified INI file path.
     *
     * @param configPath The file path to the INI configuration file.
     */
    ConfigService(const std::string& configPath);

    /**
     * @brief Gets the current application settings.
     *
     * @return A const reference to the Settings object.
     */
    const Settings& getSettings() const override { return settings_; }

    /**
     * @brief Checks if the configuration is valid.
     *
     * @return True if the configuration is valid, false otherwise.
     */
    bool isConfigValid() const override;

    /**
     * @brief Loads configuration data from the INI file.
     */
    void loadConfig() override;

    /**
     * @brief Saves configuration data to the INI file.
     *
     * @param iniData A map of section names to SettingsSection objects containing key-value pairs.
     */
    void saveConfig(const std::map<std::string, SettingsSection>& iniData) override;

    /**
     * @brief Gets the current INI configuration data.
     *
     * @return A const reference to the map of section names to SettingsSection objects.
     */
    const std::map<std::string, SettingsSection>& getIniData() const override { return iniData_; }

    /**
     * @brief Sets the INI configuration data.
     *
     * @param iniData A map of section names to SettingsSection objects containing key-value pairs.
     */
    void setIniData(const std::map<std::string, SettingsSection>& iniData) override;

    /**
     * @brief Gets the keybinding manager.
     *
     * @return A reference to the KeybindManager object.
     */
    KeybindManager& getKeybindManager() override { return keybindManager_; }

    /**
     * @brief Updates window positions in the configuration.
     *
     * @param playfieldX The x-coordinate of the playfield window.
     * @param playfieldY The y-coordinate of the playfield window.
     * @param backglassX The x-coordinate of the backglass window.
     * @param backglassY The y-coordinate of the backglass window.
     * @param dmdX The x-coordinate of the DMD window.
     * @param dmdY The y-coordinate of the DMD window.
     */
    void updateWindowPositions(int playfieldX, int playfieldY, int backglassX, int backglassY, int dmdX, int dmdY) override;

private:
    std::string configPath_;                    ///< Path to the INI configuration file.
    Settings settings_;                         ///< Current application settings.
    KeybindManager keybindManager_;             ///< Manager for keybindings.
    std::map<std::string, SettingsSection> iniData_; ///< INI configuration data.
    std::vector<std::string> originalLines_;    ///< Original lines of the INI file for preservation.
    ConfigFileHandler fileHandler_;             ///< Handler for INI file operations.
    SettingsParser parser_;                     ///< Parser for INI data into settings and keybinds.
    DefaultConfigFactory defaultFactory_;       ///< Factory for default configuration values.

    /**
     * @brief Sets default configuration settings.
     *
     * Initializes the settings_ object with default values using DefaultConfigFactory.
     */
    void setDefaultSettings();

    /**
     * @brief Initializes INI configuration data.
     *
     * Loads or creates the initial INI data structure from the configuration file.
     */
    void initializeIniData();
};

#endif // CONFIG_SERVICE_H