#ifndef ICONFIG_SERVICE_H
#define ICONFIG_SERVICE_H

#include <filesystem>
#include "config/settings.h"  // For Settings
#include "keybinds/keybind_manager.h"  // For KeybindManager
#include <map>
#include <string>

struct SettingsSection;  // Forward declare to avoid including full definition

class IConfigService {
public:
    virtual ~IConfigService() = default;
    virtual const Settings& getSettings() const = 0;
    virtual bool isConfigValid() const = 0;
    virtual void loadConfig() = 0;
    virtual void saveConfig(const std::map<std::string, SettingsSection>& iniData) = 0;
    virtual const std::map<std::string, SettingsSection>& getIniData() const = 0;
    virtual void setIniData(const std::map<std::string, SettingsSection>& iniData) = 0;
    virtual KeybindManager& getKeybindManager() = 0;
};

#endif // ICONFIG_SERVICE_H
