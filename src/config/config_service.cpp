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
    return std::filesystem::exists(settings_.VPXTablesPath) && 
        std::filesystem::exists(settings_.VPinballXPath);
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

        // Add keybindings from jsonData_
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