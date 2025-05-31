#include "config_service.h"
#include "utils/logging.h"
#include "utils/vpinballx_ini_reader.h"
#include <sstream>
#include <algorithm>
#include <cstdlib>

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

    // Store original showMetadata value to detect changes
    // bool originalShowMetadata = false;
    // auto titleDisplayIt = iniData_.find("TitleDisplay");
    // if (titleDisplayIt != iniData_.end() && titleDisplayIt->second.keyValues.size() > 0) {
    //     auto it = std::find_if(titleDisplayIt->second.keyValues.begin(),
    //                            titleDisplayIt->second.keyValues.end(),
    //                            [](const auto& pair) { return pair.first == "ShowMetadata"; });
    //     if (it != titleDisplayIt->second.keyValues.end()) {
    //         originalShowMetadata = (it->second == "true");
    //     }
    // }

    parser_.parse(iniData_, settings_, keybindManager_);
    // LOG_INFO("ConfigService: Config loaded from " << configPath_);

    // // Check if showMetadata was changed due to titleSource == "filename"
    // if (settings_.titleSource == "filename" && originalShowMetadata && !settings_.showMetadata) {
    //     fileHandler_.writeConfig(iniData_);
    //     LOG_INFO("ConfigService: Updated config.ini to reflect showMetadata=false due to titleSource=filename");
    // }

    // Apply VPinballX.ini settings if enabled
    if (settings_.useVPinballXIni) {
        std::string iniPath = std::string(std::getenv("HOME")) + "/.vpinball/VPinballX.ini";
        VPinballXIniReader iniReader(iniPath);
        auto iniSettings = iniReader.readIniSettings();
        if (iniSettings) {
            LOG_DEBUG("ConfigService: Applying VPinballX.ini settings");
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

            // Update iniData_ to reflect INI values
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

            if (iniSettings->playfieldX) updateKeyValue("PlayfieldX", std::to_string(*iniSettings->playfieldX));
            if (iniSettings->playfieldY) updateKeyValue("PlayfieldY", std::to_string(*iniSettings->playfieldY));
            if (iniSettings->playfieldWidth) updateKeyValue("PlayfieldWidth", std::to_string(*iniSettings->playfieldWidth));
            if (iniSettings->playfieldHeight) updateKeyValue("PlayfieldHeight", std::to_string(*iniSettings->playfieldHeight));
            if (iniSettings->backglassX) updateKeyValue("BackglassX", std::to_string(*iniSettings->backglassX));
            if (iniSettings->backglassY) updateKeyValue("BackglassY", std::to_string(*iniSettings->backglassY));
            if (iniSettings->backglassWidth) updateKeyValue("BackglassWidth", std::to_string(*iniSettings->backglassWidth));
            if (iniSettings->backglassHeight) updateKeyValue("BackglassHeight", std::to_string(*iniSettings->backglassHeight));
            if (iniSettings->dmdX) updateKeyValue("DMDX", std::to_string(*iniSettings->dmdX));
            if (iniSettings->dmdY) updateKeyValue("DMDY", std::to_string(*iniSettings->dmdY));
            if (iniSettings->dmdWidth) updateKeyValue("DMDWidth", std::to_string(*iniSettings->dmdWidth));
            if (iniSettings->dmdHeight) updateKeyValue("DMDHeight", std::to_string(*iniSettings->dmdHeight));

            if (iniSettings->topperX) updateKeyValue("TopperX", std::to_string(*iniSettings->topperX));
            if (iniSettings->topperY) updateKeyValue("TopperY", std::to_string(*iniSettings->topperY));
            if (iniSettings->topperWidth) updateKeyValue("TopperWidth", std::to_string(*iniSettings->topperWidth));
            if (iniSettings->topperHeight) updateKeyValue("TopperHeight", std::to_string(*iniSettings->topperHeight));

            fileHandler_.writeConfig(iniData_);
            LOG_DEBUG("ConfigService: Updated iniData_ and saved config with VPinballX.ini values");
        } else {
            LOG_DEBUG("ConfigService: VPinballX.ini not found, disabling useVPinballXIni");
            settings_.useVPinballXIni = false;
            // Update iniData_ to reflect the new setting
            auto& settingsSection = iniData_["WindowSettings"];
            auto& kv = settingsSection.keyValues;
            auto it = std::find_if(kv.begin(), kv.end(),
                                   [](const auto& pair) { return pair.first == "UseVPinballXIni"; });
            if (it != kv.end()) {
                it->second = "false";
            } else {
                kv.emplace_back("UseVPinballXIni", "false");
                settingsSection.keyToLineIndex["UseVPinballXIni"] = originalLines_.size();
            }
            fileHandler_.writeConfig(iniData_);
        }
    }
}

void ConfigService::saveConfig(const std::map<std::string, SettingsSection>& iniData) {
    fileHandler_.writeConfig(iniData);
    iniData_ = iniData;
    parser_.parse(iniData_, settings_, keybindManager_);

    // Reapply VPinballX.ini settings if enabled
    if (settings_.useVPinballXIni) {
        std::string iniPath = std::string(std::getenv("HOME")) + "/.vpinball/VPinballX.ini";
        VPinballXIniReader iniReader(iniPath);
        auto iniSettings = iniReader.readIniSettings();
        if (iniSettings) {
            LOG_DEBUG("ConfigService: Reapplying VPinballX.ini settings after save");
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

            // Update iniData_ to reflect INI values
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

            if (iniSettings->playfieldX) updateKeyValue("PlayfieldX", std::to_string(*iniSettings->playfieldX));
            if (iniSettings->playfieldY) updateKeyValue("PlayfieldY", std::to_string(*iniSettings->playfieldY));
            if (iniSettings->playfieldWidth) updateKeyValue("PlayfieldWidth", std::to_string(*iniSettings->playfieldWidth));
            if (iniSettings->playfieldHeight) updateKeyValue("PlayfieldHeight", std::to_string(*iniSettings->playfieldHeight));
            if (iniSettings->backglassX) updateKeyValue("BackglassX", std::to_string(*iniSettings->backglassX));
            if (iniSettings->backglassY) updateKeyValue("BackglassY", std::to_string(*iniSettings->backglassY));
            if (iniSettings->backglassWidth) updateKeyValue("BackglassWidth", std::to_string(*iniSettings->backglassWidth));
            if (iniSettings->backglassHeight) updateKeyValue("BackglassHeight", std::to_string(*iniSettings->backglassHeight));
            if (iniSettings->dmdX) updateKeyValue("DMDX", std::to_string(*iniSettings->dmdX));
            if (iniSettings->dmdY) updateKeyValue("DMDY", std::to_string(*iniSettings->dmdY));
            if (iniSettings->dmdWidth) updateKeyValue("DMDWidth", std::to_string(*iniSettings->dmdWidth));
            if (iniSettings->dmdHeight) updateKeyValue("DMDHeight", std::to_string(*iniSettings->dmdHeight));
            if (iniSettings->topperX) updateKeyValue("TopperX", std::to_string(*iniSettings->topperX));
            if (iniSettings->topperY) updateKeyValue("TopperY", std::to_string(*iniSettings->topperY));
            if (iniSettings->topperWidth) updateKeyValue("TopperWidth", std::to_string(*iniSettings->topperWidth));
            if (iniSettings->topperHeight) updateKeyValue("TopperHeight", std::to_string(*iniSettings->topperHeight));

            fileHandler_.writeConfig(iniData_);
            LOG_DEBUG("ConfigService: Updated iniData_ and saved config with VPinballX.ini values after save");
        } else {
            LOG_DEBUG("ConfigService: VPinballX.ini not found during save, disabling useVPinballXIni");
            settings_.useVPinballXIni = false;
            auto& settingsSection = iniData_["WindowSettings"];
            auto& kv = settingsSection.keyValues;
            auto it = std::find_if(kv.begin(), kv.end(),
                                   [](const auto& pair) { return pair.first == "UseVPinballXIni"; });
            if (it != kv.end()) {
                it->second = "false";
            } else {
                kv.emplace_back("UseVPinballXIni", "false");
                settingsSection.keyToLineIndex["UseVPinballXIni"] = originalLines_.size();
                originalLines_.push_back("UseVPinballXIni=false");
            }
            fileHandler_.writeConfig(iniData_);
        }
    }

    LOG_DEBUG("ConfigService: Config saved to " << configPath_);
}

void ConfigService::setIniData(const std::map<std::string, SettingsSection>& iniData) {
    iniData_ = iniData;
    parser_.parse(iniData_, settings_, keybindManager_);
}

void ConfigService::updateWindowPositions(int playfieldX, int playfieldY, int backglassX, int backglassY, int dmdX, int dmdY, int topperX, int topperY) {
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
    updateKeyValue("TopperX", std::to_string(topperX));
    updateKeyValue("TopperY", std::to_string(topperY));

    saveConfig(iniData_);
    LOG_INFO("ConfigService: Window positions saved: T(" << topperX << "," << topperY << "), P(" << playfieldX << "," << playfieldY << "), B("
                 << backglassX << "," << backglassY << "), D(" << dmdX << "," << dmdY << "),");
}

void ConfigService::setDefaultSettings() {
    parser_.parse({}, settings_, keybindManager_);
}

void ConfigService::initializeIniData() {
    iniData_ = defaultFactory_.getDefaultIniData();
}