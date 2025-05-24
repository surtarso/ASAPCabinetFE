#include "config/ui/config_state.h"
#include "utils/logging.h"
#include <algorithm>

const std::vector<std::string> ConfigUIState::sectionOrder_ = {
    "VPX", "DPISettings", "WindowSettings", "TitleDisplay", "CustomMedia", "MediaDimensions",  
    "Keybinds", "AudioSettings", "UISounds", "DefaultMedia", "Internal", "Table Overrides"
};

ConfigUIState::ConfigUIState(IConfigService* configService) : configService_(configService) {
    // Initialize currentSection to the first visible section in sectionOrder_
    for (const auto& section : sectionOrder_) {
        if (configService_->getIniData().count(section) > 0) {
            currentSection = section;
            break;
        }
    }
    // If no valid section found, fall back to the first in iniData
    if (currentSection.empty() && !configService_->getIniData().empty()) {
        currentSection = configService_->getIniData().begin()->first;
    }
    lastSavedIniData = configService_->getIniData();  // Store initial state
}

bool ConfigUIState::hasSectionChanged(const std::string& sectionName, const std::map<std::string, SettingsSection>& currentIniData) const {
    auto currentIt = currentIniData.find(sectionName);
    auto lastIt = lastSavedIniData.find(sectionName);

    if (currentIt == currentIniData.end() && lastIt != lastSavedIniData.end()) {
        LOG_DEBUG("ConfigUIState: " << sectionName << " section removed");
        return true;
    }
    if (currentIt != currentIniData.end() && lastIt == lastSavedIniData.end()) {
        LOG_DEBUG("ConfigUIState: " << sectionName << " section added");
        return true;
    }
    if (currentIt == currentIniData.end() && lastIt == lastSavedIniData.end()) {
        LOG_DEBUG("ConfigUIState: No " << sectionName << " section in either state");
        return false;
    }

    const auto& currentSection = currentIt->second;
    const auto& lastSection = lastIt->second;
    if (currentSection.keyValues.size() != lastSection.keyValues.size()) {
        LOG_DEBUG("ConfigUIState: " << sectionName << " key count changed: " << currentSection.keyValues.size() << " vs " << lastSection.keyValues.size());
        return true;
    }

    for (const auto& [key, value] : currentSection.keyValues) {
        const std::string currentKey = key;
        auto lastPairIt = std::find_if(lastSection.keyValues.begin(), lastSection.keyValues.end(),
                                       [currentKey](const auto& pair) { return pair.first == currentKey; });
        if (lastPairIt == lastSection.keyValues.end()) {
            LOG_DEBUG("ConfigUIState: " << sectionName << " new key: " << currentKey << "=" << value);
            return true;
        }
        if (lastPairIt->second != value) {
            LOG_DEBUG("ConfigUIState: " << sectionName << " changed: " << currentKey << " from " << lastPairIt->second << " to " << value);
            return true;
        }
    }
    LOG_DEBUG("ConfigUIState: No changes detected in " << sectionName);
    return false;
}

bool ConfigUIState::hasWindowSettingsChanged(const std::map<std::string, SettingsSection>& currentIniData) const {
    return hasSectionChanged("WindowSettings", currentIniData);
}

bool ConfigUIState::hasVisibilitySettingsChanged(const std::map<std::string, SettingsSection>& currentIniData) const {
    auto currentIt = currentIniData.find("WindowSettings");
    auto lastIt = lastSavedIniData.find("WindowSettings");

    if (currentIt == currentIniData.end() && lastIt != lastSavedIniData.end()) {
        LOG_DEBUG("ConfigUIState: WindowSettings section removed, no visibility settings to enable");
        return false;
    }
    if (currentIt != currentIniData.end() && lastIt == lastSavedIniData.end()) {
        const auto& currentSection = currentIt->second;
        for (const auto& [key, value] : currentSection.keyValues) {
            if ((key == "ShowDMD" || key == "ShowBackglass") && value == "true") {
                LOG_DEBUG("ConfigUIState: Visibility setting added: " << key << "=true");
                return true;
            }
        }
        LOG_DEBUG("ConfigUIState: WindowSettings section added, no visibility settings enabled");
        return false;
    }
    if (currentIt == currentIniData.end() && lastIt == lastSavedIniData.end()) {
        LOG_DEBUG("ConfigUIState: No WindowSettings section in either state");
        return false;
    }

    const auto& currentSection = currentIt->second;
    const auto& lastSection = lastIt->second;
    for (const auto& [key, value] : currentSection.keyValues) {
        if (key == "ShowDMD" || key == "ShowBackglass") {
            const std::string currentKey = key;
            auto lastPairIt = std::find_if(lastSection.keyValues.begin(), lastSection.keyValues.end(),
                                           [currentKey](const auto& pair) { return pair.first == currentKey; });
            if (lastPairIt == lastSection.keyValues.end()) {
                if (value == "true") {
                    LOG_DEBUG("ConfigUIState: Visibility setting added: " << currentKey << "=true");
                    return true;
                }
            } else if (lastPairIt->second != value && value == "true") {
                LOG_DEBUG("ConfigUIState: Visibility setting changed: " << currentKey << " from " << lastPairIt->second << " to true");
                return true;
            }
        }
    }
    LOG_DEBUG("ConfigUIState: No visibility settings changed to true");
    return false;
}

bool ConfigUIState::hasFontSettingsChanged(const std::map<std::string, SettingsSection>& currentIniData) const {
    auto currentIt = currentIniData.find("TitleDisplay");
    auto lastIt = lastSavedIniData.find("TitleDisplay");

    if (currentIt == currentIniData.end() && lastIt != lastSavedIniData.end()) {
        LOG_DEBUG("ConfigUIState: TitleDisplay section removed");
        return true;
    }
    if (currentIt != currentIniData.end() && lastIt == lastSavedIniData.end()) {
        LOG_DEBUG("ConfigUIState: TitleDisplay section added");
        return true;
    }
    if (currentIt == currentIniData.end() && lastIt == lastSavedIniData.end()) {
        LOG_DEBUG("ConfigUIState: No TitleDisplay section in either state");
        return false;
    }

    const auto& currentSection = currentIt->second;
    const auto& lastSection = lastIt->second;
    if (currentSection.keyValues.size() != lastSection.keyValues.size()) {
        LOG_DEBUG("ConfigUIState: TitleDisplay key count changed: " << currentSection.keyValues.size() << " vs " << lastSection.keyValues.size());
        return true;
    }

    for (const auto& [key, value] : currentSection.keyValues) {
        if (key == "FontSize" || key == "FontPath" || key == "FontColor") {
            const std::string currentKey = key;
            auto lastPairIt = std::find_if(lastSection.keyValues.begin(), lastSection.keyValues.end(),
                                           [currentKey](const auto& pair) { return pair.first == currentKey; });
            if (lastPairIt == lastSection.keyValues.end()) {
                LOG_DEBUG("ConfigUIState: Font setting added: " << currentKey << "=" << value);
                return true;
            }
            if (lastPairIt->second != value) {
                LOG_DEBUG("ConfigUIState: Font setting changed: " << currentKey << " from " << lastPairIt->second << " to " << value);
                return true;
            }
        }
    }
    LOG_DEBUG("ConfigUIState: No font settings changed");
    return false;
}

bool ConfigUIState::hasTitleDataSourceChanged(const std::map<std::string, SettingsSection>& currentIniData) const {
    auto currentIt = currentIniData.find("TitleDisplay");
    auto lastIt = lastSavedIniData.find("TitleDisplay");

    if (currentIt == currentIniData.end() || lastIt == lastSavedIniData.end()) {
        LOG_DEBUG("ConfigUIState: TitleDisplay section missing in current or last state");
        return true;
    }

    const auto& currentSection = currentIt->second;
    const auto& lastSection = lastIt->second;
    const std::string key = "TitleSource";
    auto currentPairIt = std::find_if(currentSection.keyValues.begin(), currentSection.keyValues.end(),
                                      [key](const auto& pair) { return pair.first == key; });
    auto lastPairIt = std::find_if(lastSection.keyValues.begin(), lastSection.keyValues.end(),
                                   [key](const auto& pair) { return pair.first == key; });

    if (currentPairIt == currentSection.keyValues.end() || lastPairIt == lastSection.keyValues.end()) {
        LOG_DEBUG("ConfigUIState: TitleSource key missing in current or last state");
        return true;
    }
    if (currentPairIt->second != lastPairIt->second) {
        LOG_DEBUG("ConfigUIState: TitleSource changed from " << lastPairIt->second << " to " << currentPairIt->second);
        return true;
    }
    LOG_DEBUG("ConfigUIState: No TitleSource changes");
    return false;
}

bool ConfigUIState::hasVideoBackendChanged(const std::map<std::string, SettingsSection>& currentIniData) const {
    auto currentIt = currentIniData.find("WindowSettings");
    auto lastIt = lastSavedIniData.find("WindowSettings");

    if (currentIt == currentIniData.end() || lastIt == lastSavedIniData.end()) {
        LOG_DEBUG("ConfigUIState: Internal section missing in current or last state");
        return true;
    }

    const auto& currentSection = currentIt->second;
    const auto& lastSection = lastIt->second;
    const std::string key = "VideoBackend";
    auto currentPairIt = std::find_if(currentSection.keyValues.begin(), currentSection.keyValues.end(),
                                      [key](const auto& pair) { return pair.first == key; });
    auto lastPairIt = std::find_if(lastSection.keyValues.begin(), lastSection.keyValues.end(),
                                   [key](const auto& pair) { return pair.first == key; });

    if (currentPairIt == currentSection.keyValues.end() || lastPairIt == lastSection.keyValues.end()) {
        LOG_DEBUG("ConfigUIState: VideoBackend key missing in current or last state");
        return true;
    }
    if (currentPairIt->second != lastPairIt->second) {
        LOG_DEBUG("ConfigUIState: VideoBackend changed from " << lastPairIt->second << " to " << currentPairIt->second);
        return true;
    }
    LOG_DEBUG("ConfigUIState: No VideoBackend changes");
    return false;
}

bool ConfigUIState::hasForceImagesOnlyChanged(const std::map<std::string, SettingsSection>& currentIniData) const {
    auto currentIt = currentIniData.find("MediaDimensions");
    auto lastIt = lastSavedIniData.find("MediaDimensions");

    if (currentIt == currentIniData.end() || lastIt == lastSavedIniData.end()) {
        LOG_DEBUG("ConfigUIState: MediaDimensions section missing in current or last state, assuming no change");
        return false;
    }

    const auto& currentSection = currentIt->second;
    const auto& lastSection = lastIt->second;
    const std::string key = "ForceImagesOnly";
    auto currentPairIt = std::find_if(currentSection.keyValues.begin(), currentSection.keyValues.end(),
                                      [key](const auto& pair) { return pair.first == key; });
    auto lastPairIt = std::find_if(lastSection.keyValues.begin(), lastSection.keyValues.end(),
                                   [key](const auto& pair) { return pair.first == key; });

    if (currentPairIt == currentSection.keyValues.end() || lastPairIt == lastSection.keyValues.end()) {
        LOG_DEBUG("ConfigUIState: ForceImagesOnly key missing in current or last state, assuming no change");
        return false;
    }
    if (currentPairIt->second != lastPairIt->second) {
        LOG_DEBUG("ConfigUIState: ForceImagesOnly changed from " << lastPairIt->second << " to " << currentPairIt->second);
        return true;
    }
    LOG_DEBUG("ConfigUIState: No ForceImagesOnly changes");
    return false;
}

bool ConfigUIState::hasMetadataSettingsChanged(const std::map<std::string, SettingsSection>& currentIniData) const {
    auto currentIt = currentIniData.find("TitleDisplay");
    auto lastIt = lastSavedIniData.find("TitleDisplay");

    if (currentIt == currentIniData.end() || lastIt == lastSavedIniData.end()) {
        LOG_DEBUG("ConfigUIState: TitleDisplay section missing in current or last state");
        return true;
    }

    const auto& currentSection = currentIt->second;
    const auto& lastSection = lastIt->second;
    const std::string key = "ShowMetadata";
    auto currentPairIt = std::find_if(currentSection.keyValues.begin(), currentSection.keyValues.end(),
                                      [key](const auto& pair) { return pair.first == key; });
    auto lastPairIt = std::find_if(lastSection.keyValues.begin(), lastSection.keyValues.end(),
                                   [key](const auto& pair) { return pair.first == key; });

    if (currentPairIt == currentSection.keyValues.end() || lastPairIt == lastSection.keyValues.end()) {
        LOG_DEBUG("ConfigUIState: ShowMetadata key missing in current or last state");
        return true;
    }
    if (currentPairIt->second != lastPairIt->second) {
        LOG_DEBUG("ConfigUIState: ShowMetadata changed from " << lastPairIt->second << " to " << currentPairIt->second);
        return true;
    }
    LOG_DEBUG("ConfigUIState: No ShowMetadata changes");
    return false;
}