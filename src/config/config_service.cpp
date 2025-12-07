/**
 * @file config_service.cpp
 * @brief Implementation of the ConfigService class for managing configuration settings and keybindings.
 */

#include "config/config_service.h"
#include "log/logging.h"
#include <fstream>
#include <stdexcept>
#include <filesystem>
#include <string>
#if defined(__APPLE__)
    #include <mach-o/dyld.h>
    #include <limits.h>
#endif

ConfigService::ConfigService(const std::string& configPath, IKeybindProvider* keybindProvider)
    : configPath_(configPath), keybindProvider_(keybindProvider) {
    if (!keybindProvider_) {
        LOG_ERROR("Null keybindProvider provided");
        throw std::invalid_argument("KeybindProvider cannot be null");
    }
    loadConfig();
}

const Settings& ConfigService::getSettings() const {
    return settings_;
}

Settings& ConfigService::getMutableSettings() {
    return settings_;
}

IKeybindProvider& ConfigService::getKeybindProvider() {
    if (!keybindProvider_) {
        LOG_ERROR("KeybindProvider is null");
        throw std::runtime_error("KeybindProvider is null");
    }
    return *keybindProvider_;
}

bool ConfigService::isConfigValid() const {
    LOG_INFO("Validating config file.");

    if (settings_.VPinballXPath.empty()) {
        LOG_ERROR("Config validation failed: VPinballXPath is empty.");
        return false;
    }
    if (settings_.VPXTablesPath.empty()) {
        LOG_ERROR("Config validation failed: VPXTablesPath is empty.");
        return false;
    }

    if (!std::filesystem::exists(settings_.VPinballXPath)) {
        LOG_ERROR("Config validation failed: VPinballX path does not exist: " + settings_.VPinballXPath);
        return false;
    }
    if (!std::filesystem::is_regular_file(settings_.VPinballXPath)) {
        LOG_ERROR("Config validation failed: VPinballX path is not a regular file: " + settings_.VPinballXPath);
        return false;
    }
    auto perms = std::filesystem::status(settings_.VPinballXPath).permissions();
    if ((perms & std::filesystem::perms::owner_exec) == std::filesystem::perms::none) {
        LOG_ERROR("Config validation failed: VPinballX path lacks executable permissions: " + settings_.VPinballXPath);
        return false;
    }

    if (!std::filesystem::exists(settings_.VPXTablesPath)) {
        LOG_ERROR("Config validation failed: VPXTablesPath does not exist: " + settings_.VPXTablesPath);
        return false;
    }
    if (!std::filesystem::is_directory(settings_.VPXTablesPath)) {
        LOG_ERROR("Config validation failed: VPXTablesPath is not a directory: " + settings_.VPXTablesPath);
        return false;
    }
    bool hasVpxFiles = false;
    for (const auto& entry : std::filesystem::directory_iterator(settings_.VPXTablesPath)) {
        if (entry.path().extension() == ".vpx") {
            hasVpxFiles = true;
            break;
        }
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
        LOG_ERROR("Config validation failed: No .vpx files found in VPXTablesPath: " + settings_.VPXTablesPath);
        return false;
    }

    LOG_INFO("Config validation passed.");
    return true;
}

void ConfigService::loadConfig() {
    LOG_DEBUG("Loading config from " + configPath_);
    try {
        std::ifstream file(configPath_);
        if (!file.is_open()) {
            LOG_WARN("Config file not found, using defaults");
            settings_ = Settings();
            applyPostProcessing();
            saveConfig();
            return;
        }
        jsonData_ = nlohmann::json::parse(file);
        file.close();
        settings_ = jsonData_.get<Settings>();
        applyPostProcessing();

        std::map<std::string, std::string> keybindData;
        if (jsonData_.contains("Keybinds") && jsonData_["Keybinds"].is_object()) {
            for (const auto& [action, key] : jsonData_["Keybinds"].items()) {
                if (key.is_string()) {
                    keybindData[action] = key.get<std::string>();
                }
            }
        }
        keybindProvider_->loadKeybinds(keybindData);
        settings_.keybinds_ = keybindData;

        if (settings_.useVPinballXIni) {
            std::string iniPath = settings_.vpxIniPath.empty() ?
                std::string(std::getenv("HOME")) + "/.vpinball/VPinballX.ini" : settings_.vpxIniPath;
            VPinballXIniReader iniReader(iniPath);
            auto iniSettings = iniReader.readIniSettings();
            if (iniSettings) {
                LOG_WARN("Applying VPinballX.ini settings");
                applyIniSettings(iniSettings);
                updateJsonWithIniValues(iniSettings);
            } else {
                LOG_DEBUG("VPinballX.ini not found, disabling useVPinballXIni");
                settings_.useVPinballXIni = false;
                jsonData_["WindowSettings"]["useVPinballXIni"] = false;
            }
        }
        LOG_INFO("Config loaded successfully");
    } catch (const nlohmann::json::exception& e) {
        LOG_ERROR("JSON parsing error: " + std::string(e.what()));
        settings_ = Settings();
        applyPostProcessing();
        saveConfig();
    }
}

void ConfigService::saveConfig() {
    LOG_DEBUG("Saving config to " + configPath_);
    try {
        jsonData_ = settings_;
        std::map<std::string, std::string> keybindData;
        keybindProvider_->saveKeybinds(keybindData);
        settings_.keybinds_ = keybindData;
        jsonData_["Keybinds"] = keybindData;

        if (settings_.useVPinballXIni) {
            std::string iniPath = settings_.vpxIniPath.empty() ?
                std::string(std::getenv("HOME")) + "/.vpinball/VPinballX.ini" : settings_.vpxIniPath;
            VPinballXIniReader iniReader(iniPath);
            auto iniSettings = iniReader.readIniSettings();
            if (iniSettings) {
                LOG_WARN("Reapplying VPinballX.ini settings after save");
                applyIniSettings(iniSettings);
                updateJsonWithIniValues(iniSettings);
            } else {
                LOG_WARN("VPinballX.ini not found during save, disabling useVPinballXIni");
                settings_.useVPinballXIni = false;
                jsonData_["WindowSettings"]["useVPinballXIni"] = false;
            }
        }

        std::ofstream tempFile(configPath_ + ".tmp");
        if (!tempFile.is_open()) {
            LOG_ERROR("Failed to open temp file for writing: " + configPath_ + ".tmp");
            return;
        }
        tempFile << jsonData_.dump(4);
        tempFile.close();
        std::filesystem::rename(configPath_ + ".tmp", configPath_);
        LOG_INFO("Config saved successfully");
    } catch (const std::exception& e) {
        LOG_ERROR("Error saving config: " + std::string(e.what()));
        if (std::filesystem::exists(configPath_ + ".tmp")) {
            std::filesystem::remove(configPath_ + ".tmp");
        }
    }
}

void ConfigService::applyPostProcessing() {
    std::string exeDir;
    char path[PATH_MAX];

    #if defined(__APPLE__)
        // macOS: _NSGetExecutablePath + realpath
        uint32_t size = sizeof(path);

        // Try with static buffer
        if (_NSGetExecutablePath(path, &size) == 0) {
            char resolved[PATH_MAX];
            if (realpath(path, resolved) != nullptr) {
                // Executable absolute path, get directory
                exeDir = std::filesystem::path(resolved).parent_path().string() + "/";
            } else {
                // Couldn't resolve symlinks, fallback to raw
                exeDir = std::filesystem::path(path).parent_path().string() + "/";
            }
        } else {
            // Buffer too small, allocate exactly the required size
            std::string dynamicPath(size, '\0');
            if (_NSGetExecutablePath(dynamicPath.data(), &size) == 0) {
                char resolved[PATH_MAX];
                if (realpath(dynamicPath.c_str(), resolved) != nullptr) {
                    exeDir = std::filesystem::path(resolved).parent_path().string() + "/";
                } else {
                    exeDir = std::filesystem::path(dynamicPath).parent_path().string() + "/";
                }
            } else {
                // Completely failed — fallback to CWD
                exeDir = std::filesystem::current_path().string() + "/";
            }
        }
    #else
        // Linux: /proc/self/exe
        ssize_t count = readlink("/proc/self/exe", path, sizeof(path) - 1);
        if (count != -1) {
            path[count] = '\0';
            exeDir = std::filesystem::path(path).parent_path().string() + "/";
        } else {
            exeDir = std::filesystem::current_path().string() + "/";
            LOG_WARN("Failed to resolve executable path, using current directory: " + exeDir);
        }
    #endif

    settings_.applyPostProcessing(exeDir);
}

void ConfigService::applyIniSettings(const std::optional<VPinballXIniSettings>& iniSettings) {
    if (iniSettings) {
        if (iniSettings->playfieldX) settings_.playfieldX = *iniSettings->playfieldX;
        if (iniSettings->playfieldY) settings_.playfieldY = *iniSettings->playfieldY;
        if (iniSettings->playfieldWidth) settings_.playfieldWindowWidth = *iniSettings->playfieldWidth;
        if (iniSettings->playfieldHeight) settings_.playfieldWindowHeight = *iniSettings->playfieldHeight;

        if (iniSettings->backglassX) settings_.backglassX = *iniSettings->backglassX;
        if (iniSettings->backglassY) settings_.backglassY = *iniSettings->backglassY;
        if (iniSettings->backglassWidth) settings_.backglassWindowWidth = *iniSettings->backglassWidth;
        if (iniSettings->backglassHeight) settings_.backglassWindowHeight = *iniSettings->backglassHeight;

        if (iniSettings->dmdX) settings_.dmdX = *iniSettings->dmdX;
        if (iniSettings->dmdY) settings_.dmdY = *iniSettings->dmdY;
        if (iniSettings->dmdWidth) settings_.dmdWindowWidth = *iniSettings->dmdWidth;
        if (iniSettings->dmdHeight) settings_.dmdWindowHeight = *iniSettings->dmdHeight;

        if (iniSettings->topperX) settings_.topperWindowX = *iniSettings->topperX;
        if (iniSettings->topperY) settings_.topperWindowY = *iniSettings->topperY;
        if (iniSettings->topperWidth) settings_.topperWindowWidth = *iniSettings->topperWidth;
        if (iniSettings->topperHeight) settings_.topperWindowHeight = *iniSettings->topperHeight;
    }
}

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

    LOG_INFO("Window setup saved: P(" + std::to_string(playfieldX) + "," + std::to_string(playfieldY) + "," +
             std::to_string(playfieldWidth) + "," + std::to_string(playfieldHeight) + "), B(" +
             std::to_string(backglassX) + "," + std::to_string(backglassY) + "," +
             std::to_string(backglassWidth) + "," + std::to_string(backglassHeight) + "), D(" +
             std::to_string(dmdX) + "," + std::to_string(dmdY) + "," +
             std::to_string(dmdWidth) + "," + std::to_string(dmdHeight) + "), T(" +
             std::to_string(topperX) + "," + std::to_string(topperY) + "," +
             std::to_string(topperWidth) + "," + std::to_string(topperHeight) + ")");
}
