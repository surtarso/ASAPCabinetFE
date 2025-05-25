/**
 * @file settings_section.h
 * @brief Defines the SettingsSection struct for storing configuration section data in ASAPCabinetFE.
 *
 * This header provides the SettingsSection struct, which stores key-value pairs for a
 * configuration section parsed from an INI file, along with a mapping of keys to their
 * line indices for efficient lookup.
 */

#ifndef SETTINGS_SECTION_H
#define SETTINGS_SECTION_H

#include <string>
#include <vector>
#include <unordered_map>

/**
 * @struct SettingsSection
 * @brief Stores key-value pairs for a configuration section.
 *
 * This struct represents a single section of an INI configuration file, containing a
 * list of key-value pairs and a map for quick key-to-line index lookup. It is used by
 * SettingsParser to process configuration data.
 */
struct SettingsSection {
    std::vector<std::pair<std::string, std::string>> keyValues; ///< List of key-value pairs in the section.
    std::unordered_map<std::string, size_t> keyToLineIndex;     ///< Map of keys to their indices in keyValues.
};

#endif // SETTINGS_SECTION_H