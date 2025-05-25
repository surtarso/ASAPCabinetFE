/**
 * @file default_config_factory.cpp
 * @brief Implements the DefaultConfigFactory class for generating default configurations in ASAPCabinetFE.
 *
 * This file provides the implementation of the DefaultConfigFactory class, which
 * generates default INI configuration data and settings using a ConfigSchema. It
 * populates SettingsSection objects with default key-value pairs and initializes
 * Settings objects with default values.
 */

#include "default_config_factory.h"
#include <sstream>

/**
 * @brief Constructs a DefaultConfigFactory instance.
 *
 * Initializes the factory with a ConfigSchema for defining default settings.
 */
DefaultConfigFactory::DefaultConfigFactory() : schema_() {}

/**
 * @brief Gets default INI configuration data.
 *
 * Generates a map of section names to SettingsSection objects containing default
 * key-value pairs for settings (from ConfigSchema) and keybindings (hardcoded).
 * Formats values based on their type (string, int, float, bool, SDLColor) and
 * populates key-to-line index mappings.
 *
 * @return A map of section names to SettingsSection objects.
 */
std::map<std::string, SettingsSection> DefaultConfigFactory::getDefaultIniData() const {
    std::map<std::string, SettingsSection> iniData;

    for (const auto& var : schema_.getVariables()) {
        auto& section = iniData[var.iniSection];
        std::string value;
        switch (var.type) {
            case ConfigSchema::Type::String:
                value = std::get<std::string>(var.defaultValue);
                break;
            case ConfigSchema::Type::Int:
                value = std::to_string(std::get<int>(var.defaultValue));
                break;
            case ConfigSchema::Type::Float:
                value = std::to_string(std::get<float>(var.defaultValue));
                break;
            case ConfigSchema::Type::Bool:
                value = std::get<bool>(var.defaultValue) ? "true" : "false";
                break;
            case ConfigSchema::Type::SDLColor: {
                const auto& color = std::get<SDL_Color>(var.defaultValue);
                std::ostringstream oss;
                oss << static_cast<int>(color.r) << "," << static_cast<int>(color.g) << ","
                    << static_cast<int>(color.b) << "," << static_cast<int>(color.a);
                value = oss.str();
                break;
            }
        }
        section.keyValues.emplace_back(var.iniKey, value);
    }

    for (auto& [sectionName, section] : iniData) {
        for (size_t i = 0; i < section.keyValues.size(); ++i) {
            section.keyToLineIndex[section.keyValues[i].first] = i;
        }
    }

    // Add keybinds (managed by KeybindManager, not Settings)
    auto& keybinds = iniData["Keybinds"];
    keybinds.keyValues = {
        {"PreviousTable", "Left Shift"},
        {"NextTable", "Right Shift"},
        {"FastPrevTable", "Left Ctrl"},
        {"FastNextTable", "Right Ctrl"},
        {"JumpNextLetter", "Slash"},
        {"JumpPrevLetter", "Z"},
        {"RandomTable", "R"},
        {"LaunchTable", "Return"},
        {"ToggleConfig", "C"},
        {"Quit", "Q"},
        {"ConfigSave", "Space"},
        {"ConfigClose", "Q"},
        {"ScreenshotMode", "S"},
        {"ScreenshotKey", "S"},
        {"ScreenshotQuit", "Q"}
    };
    for (size_t i = 0; i < keybinds.keyValues.size(); ++i) {
        keybinds.keyToLineIndex[keybinds.keyValues[i].first] = i;
    }

    return iniData;
}

/**
 * @brief Populates a Settings object with default values.
 *
 * Initializes the provided Settings object with default values defined by the
 * ConfigSchema, using the schema's default setters to assign values to Settings fields.
 *
 * @param settings The Settings object to populate with default values.
 */
void DefaultConfigFactory::getDefaultSettings(Settings& settings) const {
    // Initialize all fields to default values (e.g., empty strings, zeros)
    settings = Settings{};

    for (const auto& var : schema_.getVariables()) {
        var.defaultSetter(settings, var.defaultValue);
    }
}