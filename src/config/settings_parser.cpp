#include "settings_parser.h"
#include "utils/logging.h"
#include <sstream>
#include <algorithm>

SettingsParser::SettingsParser(const std::string& configPath) 
    : configPath_(configPath), defaultFactory_(), schema_() {}

void SettingsParser::parse(const std::map<std::string, SettingsSection>& iniData, Settings& settings, KeybindManager& keybindManager) const {
    // Flatten iniData for easier lookup
    std::map<std::string, std::map<std::string, std::string>> config;
    for (const auto& [section, configSection] : iniData) {
        for (const auto& kv : configSection.keyValues) {
            config[section][kv.first] = kv.second;
        }
    }

    // Initialize settings with defaults
    defaultFactory_.getDefaultSettings(settings);

    // Resolve relative paths
    std::string exeDir = configPath_.substr(0, configPath_.find_last_of('/') + 1);
    auto resolvePath = [&](const std::string& value, const std::string& defaultPath) {
        if (value.empty()) return exeDir + defaultPath;
        if (value.find('/') == 0 || value.find('\\') == 0) return value;
        return exeDir + value;
    };

    // Parse settings using ConfigSchema
    for (const auto& var : schema_.getVariables()) {
        std::string value = config[var.iniSection][var.iniKey];
        if (value.empty()) continue; // Use default from getDefaultSettings

        try {
            if (var.needsPathResolution && var.type == ConfigSchema::Type::String) {
                value = resolvePath(value, std::get<std::string>(var.defaultValue));
            }
            var.parseSetter(settings, value);
        } catch (const std::exception& e) {
            LOG_ERROR("SettingsParser: Failed to parse " << var.iniSection << "." << var.iniKey << ": " << e.what());
        }
    }

    // Apply post-processing
    for (const auto& var : schema_.getVariables()) {
        if (var.postProcess == ConfigSchema::PostProcess::DpiScaleFontSize && settings.enableDpiScaling) {
            settings.fontSize = static_cast<int>(settings.fontSize * settings.dpiScale);
        }
    }

    // Load keybinds
    keybindManager.loadKeybinds(config["Keybinds"]);
}