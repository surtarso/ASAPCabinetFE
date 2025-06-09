#include "config_service.h"
#include "utils/logging.h"
#include <fstream>
#include <stdexcept>

ConfigService::ConfigService(const std::string& configPath)
    : configPath_(configPath), keybindManager_() {
    loadConfig();
}

const Settings& ConfigService::getSettings() const {
    return settings_;
}

bool ConfigService::isConfigValid() const {
    LOG_DEBUG("Validating config file.");

    // Check for empty or unresolved paths
    if (settings_.VPinballXPath.empty()) {
        LOG_ERROR("Config validation failed: VPinballXPath is empty.");
        return false;
    }
    if (settings_.VPXTablesPath.empty()) {
        LOG_ERROR("Config validation failed: VPXTablesPath is empty.");
        return false;
    }

    // Validate VPinballX path as an executable
    if (!std::filesystem::exists(settings_.VPinballXPath)) {
        LOG_ERROR("Config validation failed: VPinballX path does not exist: " << settings_.VPinballXPath);
        return false;
    }
    if (!std::filesystem::is_regular_file(settings_.VPinballXPath)) {
        LOG_ERROR("Config validation failed: VPinballX path is not a regular file: " << settings_.VPinballXPath);
        return false;
    }
    auto perms = std::filesystem::status(settings_.VPinballXPath).permissions();
    if ((perms & std::filesystem::perms::owner_exec) == std::filesystem::perms::none) {
        LOG_ERROR("Config validation failed: VPinballX path lacks executable permissions: " << settings_.VPinballXPath);
        return false;
    }

    // Validate VPXTablesPath contains .vpx files (depth 1)
    if (!std::filesystem::exists(settings_.VPXTablesPath)) {
        LOG_ERROR("Config validation failed: VPXTablesPath does not exist: " << settings_.VPXTablesPath);
        return false;
    }
    if (!std::filesystem::is_directory(settings_.VPXTablesPath)) {
        LOG_ERROR("Config validation failed: VPXTablesPath is not a directory: " << settings_.VPXTablesPath);
        return false;
    }
    bool hasVpxFiles = false;
    // Check root directory
    for (const auto& entry : std::filesystem::directory_iterator(settings_.VPXTablesPath)) {
        if (entry.path().extension() == ".vpx") {
            hasVpxFiles = true;
            break;
        }
        // Check one level of subdirectories if no .vpx found yet
        if (!hasVpxFiles && entry.is_directory()) {
            for (const auto& subEntry : std::filesystem::directory_iterator(entry.path())) {
                if (subEntry.path().extension() == ".vpx") {
                    hasVpxFiles = true;
                    break;
                }
            }
            if (hasVpxFiles) break;
        }
    }
    if (!hasVpxFiles) {
        LOG_ERROR("Config validation failed: No .vpx files found in VPXTablesPath: " << settings_.VPXTablesPath);
        return false;
    }

    LOG_DEBUG("Config validation passed.");
    return true;
}

void ConfigService::loadConfig() {
    LOG_DEBUG("ConfigService: Loading config from " << configPath_);

    try {
        std::ifstream file(configPath_);
        if (!file.is_open()) {
            LOG_DEBUG("ConfigService: Config file not found, using defaults");
            settings_ = Settings();  // Use default settings
            applyPostProcessing();
            saveConfig();  // Save defaults to file
            return;
        }

        jsonData_ = nlohmann::json::parse(file);
        file.close();

        settings_ = jsonData_.get<Settings>();
        applyPostProcessing();

        // Load keybindings into KeybindManager
        if (jsonData_.contains("Keybinds") && jsonData_["Keybinds"].is_object()) {
            for (const auto& [action, key] : jsonData_["Keybinds"].items()) {
                if (key.is_number_integer()) {
                    keybindManager_.setKey(action, static_cast<SDL_Keycode>(key.get<int>()));
                } else if (key.is_string()) {
                    SDL_Keycode code = SDL_GetKeyFromName(key.get<std::string>().c_str());
                    if (code != SDLK_UNKNOWN) {
                        keybindManager_.setKey(action, code);
                    }
                }
            }
        }

        // Apply VPinballX.ini settings if enabled
        if (settings_.useVPinballXIni) {
            std::string iniPath;
            if (settings_.vpxIniPath.empty()) {
                // Use vpx defaults if no custom path was given
                iniPath = std::string(std::getenv("HOME")) + "/.vpinball/VPinballX.ini";
            } else {
                iniPath = settings_.vpxIniPath;
            }
            VPinballXIniReader iniReader(iniPath);
            auto iniSettings = iniReader.readIniSettings();
            if (iniSettings) {
                LOG_DEBUG("ConfigService: Applying VPinballX.ini settings");
                applyIniSettings(iniSettings);
                updateJsonWithIniValues(iniSettings);
            } else {
                LOG_DEBUG("ConfigService: VPinballX.ini not found, disabling useVPinballXIni");
                settings_.useVPinballXIni = false;
                jsonData_["WindowSettings"]["useVPinballXIni"] = false;
            }
        }

        LOG_DEBUG("ConfigService: Config loaded successfully");
    } catch (const nlohmann::json::exception& e) {
        LOG_ERROR("ConfigService: JSON parsing error: " << e.what());
        settings_ = Settings();  // Fallback to defaults
        applyPostProcessing();
    }
}

void ConfigService::saveConfig() {
    LOG_DEBUG("ConfigService: Saving config to " << configPath_);

    try {
        // Update jsonData_ from settings_
        jsonData_ = settings_;

        // Add keybindings from settings_
        if (jsonData_.contains("Keybinds")) {
            jsonData_["Keybinds"] = jsonData_["Keybinds"]; // Preserve existing Keybinds
        } else {
            jsonData_["Keybinds"] = nlohmann::json::object();
        }
        for (const auto& [action, key] : settings_.keybinds_) {
            std::string keyName = SDL_GetKeyName(key);
            if (!keyName.empty()) {
                jsonData_["Keybinds"][action] = keyName;
            }
        }

        // Reapply VPinballX.ini settings if enabled
        if (settings_.useVPinballXIni) {
            std::string iniPath = settings_.vpxIniPath.empty() ?
                std::string(std::getenv("HOME")) + "/.vpinball/VPinballX.ini" : settings_.vpxIniPath;
            VPinballXIniReader iniReader(iniPath);
            auto iniSettings = iniReader.readIniSettings();
            if (iniSettings) {
                LOG_DEBUG("ConfigService: Reapplying VPinballX.ini settings after save");
                applyIniSettings(iniSettings);
                updateJsonWithIniValues(iniSettings);
            } else {
                LOG_DEBUG("ConfigService: VPinballX.ini not found during save, disabling useVPinballXIni");
                settings_.useVPinballXIni = false;
                jsonData_["WindowSettings"]["useVPinballXIni"] = false;
            }
        }

        // Write to file
        std::ofstream file(configPath_);
        if (!file.is_open()) {
            LOG_ERROR("ConfigService: Failed to open config file for writing: " << configPath_);
            return;
        }

        file << jsonData_.dump(4);  // Pretty print with 4-space indentation
        file.close();

        LOG_DEBUG("ConfigService: Config saved successfully");
    } catch (const std::exception& e) {
        LOG_ERROR("ConfigService: Error saving config: " << e.what());
    }
}

KeybindManager& ConfigService::getKeybindManager() {
    return keybindManager_;
}

void ConfigService::applyPostProcessing() {
    std::string exeDir = std::filesystem::current_path().string() + "/";
    settings_.applyPostProcessing(exeDir);
}

// Helper function to apply VPinballX.ini settings to Settings
void ConfigService::applyIniSettings(const std::optional<VPinballXIniSettings>& iniSettings) {
    if (iniSettings) {
        // Playfield settings
        if (iniSettings->playfieldX) settings_.playfieldX = *iniSettings->playfieldX;
        if (iniSettings->playfieldY) settings_.playfieldY = *iniSettings->playfieldY;
        if (iniSettings->playfieldWidth) settings_.playfieldWindowWidth = *iniSettings->playfieldWidth;
        if (iniSettings->playfieldHeight) settings_.playfieldWindowHeight = *iniSettings->playfieldHeight;

        // Backglass settings
        if (iniSettings->backglassX) settings_.backglassX = *iniSettings->backglassX;
        if (iniSettings->backglassY) settings_.backglassY = *iniSettings->backglassY;
        if (iniSettings->backglassWidth) settings_.backglassWindowWidth = *iniSettings->backglassWidth;
        if (iniSettings->backglassHeight) settings_.backglassWindowHeight = *iniSettings->backglassHeight;

        // DMD settings
        if (iniSettings->dmdX) settings_.dmdX = *iniSettings->dmdX;
        if (iniSettings->dmdY) settings_.dmdY = *iniSettings->dmdY;
        if (iniSettings->dmdWidth) settings_.dmdWindowWidth = *iniSettings->dmdWidth;
        if (iniSettings->dmdHeight) settings_.dmdWindowHeight = *iniSettings->dmdHeight;

        // Topper settings
        if (iniSettings->topperX) settings_.topperWindowX = *iniSettings->topperX;
        if (iniSettings->topperY) settings_.topperWindowY = *iniSettings->topperY;
        if (iniSettings->topperWidth) settings_.topperWindowWidth = *iniSettings->topperWidth;
        if (iniSettings->topperHeight) settings_.topperWindowHeight = *iniSettings->topperHeight;
    }
}

// Helper function to update JSON with VPinballX.ini values
void ConfigService::updateJsonWithIniValues(const std::optional<VPinballXIniSettings>& iniSettings) {
    auto& windowSettings = jsonData_["WindowSettings"];
    auto updateJsonValue = [&](const std::string& key, const std::optional<int>& value) {
        if (value) {
            windowSettings[key] = *value;
        }
    };

    updateJsonValue("playfieldX", iniSettings->playfieldX);
    updateJsonValue("playfieldY", iniSettings->playfieldY);
    updateJsonValue("playfieldWindowWidth", iniSettings->playfieldWidth);
    updateJsonValue("playfieldWindowHeight", iniSettings->playfieldHeight);
    updateJsonValue("backglassX", iniSettings->backglassX);
    updateJsonValue("backglassY", iniSettings->backglassY);
    updateJsonValue("backglassWindowWidth", iniSettings->backglassWidth);
    updateJsonValue("backglassWindowHeight", iniSettings->backglassHeight);
    updateJsonValue("dmdX", iniSettings->dmdX);
    updateJsonValue("dmdY", iniSettings->dmdY);
    updateJsonValue("dmdWindowWidth", iniSettings->dmdWidth);
    updateJsonValue("dmdWindowHeight", iniSettings->dmdHeight);
    updateJsonValue("topperWindowX", iniSettings->topperX);
    updateJsonValue("topperWindowY", iniSettings->topperY);
    updateJsonValue("topperWindowWidth", iniSettings->topperWidth);
    updateJsonValue("topperWindowHeight", iniSettings->topperHeight);
}

// Helper for saving windows size and position as they are when used
void ConfigService::updateWindowSetup(int& playfieldX, int& playfieldY, int& playfieldWidth, int& playfieldHeight,
                            int& backglassX, int& backglassY, int& backglassWidth, int& backglassHeight,
                            int& dmdX, int& dmdY, int& dmdWidth, int& dmdHeight,
                            int& topperX, int& topperY, int& topperWidth, int& topperHeight) {

    settings_.playfieldX = playfieldX;
    settings_.playfieldY = playfieldY;
    settings_.playfieldWindowWidth = playfieldWidth;
    settings_.playfieldWindowHeight = playfieldHeight;

    settings_.backglassX = backglassX;
    settings_.backglassY = backglassY;
    settings_.backglassWindowWidth = backglassWidth;
    settings_.backglassWindowHeight = backglassHeight;

    settings_.dmdX = dmdX;
    settings_.dmdY = dmdY;
    settings_.dmdWindowWidth = dmdWidth;
    settings_.dmdWindowHeight = dmdHeight;

    settings_.topperWindowX = topperX;
    settings_.topperWindowY = topperY;
    settings_.topperWindowWidth = topperWidth;
    settings_.topperWindowHeight = topperHeight;

    saveConfig();

    LOG_INFO("Window setup saved: P(" << playfieldX << "," << playfieldY << "," << playfieldWidth << "," << playfieldHeight
             << "), B(" << backglassX << "," << backglassY << "," << backglassWidth << "," << backglassHeight
             << "), D(" << dmdX << "," << dmdY << "," << dmdWidth << "," << dmdHeight
             << "), T(" << topperX << "," << topperY << "," << topperWidth << "," << topperHeight << ")");
}