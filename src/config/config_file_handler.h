/**
 * @file config_file_handler.h
 * @brief Defines the ConfigFileHandler class for reading and writing INI files in ASAPCabinetFE.
 *
 * This header provides the ConfigFileHandler class, which handles file operations for
 * reading and writing INI configuration data. It parses INI files into SettingsSection
 * objects and writes configuration data back to the file.
 */

#ifndef CONFIG_FILE_HANDLER_H
#define CONFIG_FILE_HANDLER_H

#include "config/settings_section.h"
#include <map>
#include <string>
#include <vector>

/**
 * @class ConfigFileHandler
 * @brief Manages reading and writing of INI configuration files.
 *
 * This class reads INI configuration files into a map of SettingsSection objects and
 * writes configuration data back to the file. It preserves original file lines during
 * reading and is used by ConfigService for configuration file operations.
 */
class ConfigFileHandler {
public:
    /**
     * @brief Constructs a ConfigFileHandler instance.
     *
     * Initializes the handler with the path to the INI configuration file.
     *
     * @param configPath The file path to the INI configuration file.
     */
    ConfigFileHandler(const std::string& configPath);

    /**
     * @brief Reads the INI configuration file.
     *
     * Parses the INI file into a map of section names to SettingsSection objects and
     * stores the original file lines for preservation.
     *
     * @param originalLines A vector to store the original lines of the INI file.
     * @return A map of section names to SettingsSection objects containing key-value pairs.
     */
    std::map<std::string, SettingsSection> readConfig(std::vector<std::string>& originalLines);

    /**
     * @brief Writes configuration data to the INI file.
     *
     * Saves the provided configuration data to the INI file, maintaining the structure
     * of sections and key-value pairs.
     *
     * @param iniData A map of section names to SettingsSection objects containing key-value pairs.
     */
    void writeConfig(const std::map<std::string, SettingsSection>& iniData);

private:
    std::string configPath_; ///< Path to the INI configuration file.
};

#endif // CONFIG_FILE_HANDLER_H