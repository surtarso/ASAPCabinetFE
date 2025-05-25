/**
 * @file default_config_factory.h
 * @brief Defines the DefaultConfigFactory class for generating default configurations in ASAPCabinetFE.
 *
 * This header provides the DefaultConfigFactory class, which generates default INI
 * configuration data and settings based on a configuration schema. It is used to
 * initialize default values for application settings and keybindings.
 */

#ifndef DEFAULT_CONFIG_FACTORY_H
#define DEFAULT_CONFIG_FACTORY_H

#include "config/settings.h"
#include "config/settings_section.h"
#include "config/config_schema.h"
#include <map>
#include <string>

/**
 * @class DefaultConfigFactory
 * @brief Generates default INI configuration data and settings.
 *
 * This class uses a ConfigSchema to create default configuration data as a map of
 * SettingsSection objects and to populate a Settings object with default values. It
 * is used by SettingsParser and ConfigService to initialize configurations when no
 * INI file exists or for resetting to defaults.
 */
class DefaultConfigFactory {
public:
    /**
     * @brief Constructs a DefaultConfigFactory instance.
     *
     * Initializes the factory with a ConfigSchema for defining default settings.
     */
    DefaultConfigFactory();

    /**
     * @brief Gets default INI configuration data.
     *
     * Generates a map of section names to SettingsSection objects containing default
     * key-value pairs, including settings and keybindings.
     *
     * @return A map of section names to SettingsSection objects.
     */
    std::map<std::string, SettingsSection> getDefaultIniData() const;

    /**
     * @brief Populates a Settings object with default values.
     *
     * Initializes the provided Settings object with default values defined by the
     * ConfigSchema.
     *
     * @param settings The Settings object to populate with default values.
     */
    void getDefaultSettings(Settings& settings) const;

private:
    ConfigSchema schema_; ///< Configuration schema defining default settings and types.
};

#endif // DEFAULT_CONFIG_FACTORY_H