#ifndef CONFIG_STATE_H
#define CONFIG_STATE_H

#include <vector>
#include <string>
#include <map>
#include "config/settings_section.h"
#include "config/iconfig_service.h"

class ConfigUIState {
public:
    ConfigUIState(IConfigService* configService);  // Initialize with config service
    std::string currentSection;
    bool hasChanges = false;
    float saveMessageTimer = 0.0f;
    std::map<std::string, bool> showPicker;
    std::map<std::string, SettingsSection> lastSavedIniData;

    bool hasSectionChanged(const std::string& sectionName, const std::map<std::string, SettingsSection>& currentIniData) const;
    bool hasWindowSettingsChanged(const std::map<std::string, SettingsSection>& currentIniData) const;
    bool hasVisibilitySettingsChanged(const std::map<std::string, SettingsSection>& currentIniData) const;
    bool hasFontSettingsChanged(const std::map<std::string, SettingsSection>& currentIniData) const;
    bool hasTitleDataSourceChanged(const std::map<std::string, SettingsSection>& currentIniData) const;
    bool hasVideoBackendChanged(const std::map<std::string, SettingsSection>& currentIniData) const;

private:
    static const std::vector<std::string> sectionOrder_;
    IConfigService* configService_;
};

#endif // CONFIG_STATE_H