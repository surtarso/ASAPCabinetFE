/**
 * @file settings_parser.h
 * @brief Defines the SettingsParser class for parsing configuration data in ASAPCabinetFE.
 *
 * This header provides the SettingsParser class, which parses INI configuration data
 * into Settings and KeybindManager objects. It uses DefaultConfigFactory for default
 * values and ConfigSchema for validation.
 */

#ifndef SETTINGS_PARSER_H
#define SETTINGS_PARSER_H

#include "config/settings.h"
#include "config/settings_section.h"
#include "config/default_config_factory.h"
#include "config/config_schema.h"
#include "keybinds/keybind_manager.h"
#include <map>
#include <string>

/**
 * @class SettingsParser
 * @brief Parses INI configuration data into settings and keybinds.
 *
 * This class processes INI configuration data from a file, populating a Settings
 * object with configuration values and a KeybindManager with keybinding data. It
 * uses DefaultConfigFactory to provide default values and ConfigSchema to validate
 * the configuration structure.
 */
class SettingsParser {
public:
    /**
     * @brief Constructs a SettingsParser instance.
     *
     * Initializes the parser with the path to the configuration file.
     *
     * @param configPath The file path to the INI configuration file.
     */
    SettingsParser(const std::string& configPath);

    /**
     * @brief Parses INI data into settings and keybinds.
     *
     * Processes the provided INI data, populating the Settings object with configuration
     * values and the KeybindManager with keybinding data.
     *
     * @param iniData A map of section names to SettingsSection objects containing key-value pairs.
     * @param settings The Settings object to populate with configuration values.
     * @param keybindManager The KeybindManager to populate with keybinding data.
     */
    void parse(const std::map<std::string, SettingsSection>& iniData, Settings& settings, KeybindManager& keybindManager) const;

private:
    std::string configPath_;            ///< Path to the INI configuration file.
    DefaultConfigFactory defaultFactory_; ///< Factory for default configuration values.
    ConfigSchema schema_;                ///< Schema for validating configuration data.
};

#endif // SETTINGS_PARSER_H