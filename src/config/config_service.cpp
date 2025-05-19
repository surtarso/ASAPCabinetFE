#include "config_service.h"
#include "utils/logging.h"
#include <sstream>
#include <algorithm>

ConfigService::ConfigService(const std::string& configPath) 
    : configPath_(configPath), keybindManager_(), fileHandler_(configPath), parser_(configPath), defaultFactory_() {
    loadConfig();
}

bool ConfigService::isConfigValid() const {
    return std::filesystem::exists(settings_.VPXTablesPath) && 
           std::filesystem::exists(settings_.VPinballXPath);
}

void ConfigService::loadConfig() {
    iniData_ = fileHandler_.readConfig(originalLines_);
    if (iniData_.empty()) {
        LOG_INFO("ConfigService: Could not open " << configPath_ << ". Using defaults and creating config.ini.");
        setDefaultSettings();
        initializeIniData();
        fileHandler_.writeConfig(iniData_);
    }
    parser_.parse(iniData_, settings_, keybindManager_);
    LOG_INFO("ConfigService: Config loaded from " << configPath_);
}

void ConfigService::saveConfig(const std::map<std::string, SettingsSection>& iniData) {
    fileHandler_.writeConfig(iniData);
    iniData_ = iniData;
    parser_.parse(iniData_, settings_, keybindManager_);
    LOG_DEBUG("ConfigService: Config saved to " << configPath_);
}

void ConfigService::setIniData(const std::map<std::string, SettingsSection>& iniData) {
    iniData_ = iniData;
    parser_.parse(iniData_, settings_, keybindManager_);
}

void ConfigService::updateWindowPositions(int playfieldX, int playfieldY, int backglassX, int backglassY, int dmdX, int dmdY) {
    auto& windowSettings = iniData_["WindowSettings"];
    auto updateKeyValue = [&](const std::string& key, const std::string& value) {
        auto& kv = windowSettings.keyValues;
        auto it = std::find_if(kv.begin(), kv.end(),
                               [&key](const auto& pair) { return pair.first == key; });
        if (it != kv.end()) {
            it->second = value;
        } else {
            kv.emplace_back(key, value);
            windowSettings.keyToLineIndex[key] = originalLines_.size();
        }
    };

    updateKeyValue("PlayfieldX", std::to_string(playfieldX));
    updateKeyValue("PlayfieldY", std::to_string(playfieldY));
    updateKeyValue("BackglassX", std::to_string(backglassX));
    updateKeyValue("BackglassY", std::to_string(backglassY));
    updateKeyValue("DMDX", std::to_string(dmdX));
    updateKeyValue("DMDY", std::to_string(dmdY));

    saveConfig(iniData_);
    LOG_INFO("ConfigService: Window positions saved: P(" << playfieldX << "," << playfieldY << "), B("
                 << backglassX << "," << backglassY << "), D(" << dmdX << "," << dmdY << ")");
}

void ConfigService::setDefaultSettings() {
    parser_.parse({}, settings_, keybindManager_);
}

void ConfigService::initializeIniData() {
    iniData_ = defaultFactory_.getDefaultIniData();
}