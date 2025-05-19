#include "default_config_factory.h"
#include <sstream>

DefaultConfigFactory::DefaultConfigFactory() : schema_() {}

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

void DefaultConfigFactory::getDefaultSettings(Settings& settings) const {
    // Initialize all fields to default values (e.g., empty strings, zeros)
    settings = Settings{};

    for (const auto& var : schema_.getVariables()) {
        var.defaultSetter(settings, var.defaultValue);
    }
}