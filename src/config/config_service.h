#ifndef CONFIG_SERVICE_H
#define CONFIG_SERVICE_H

#include "config/settings.h"
#include "keybinds/keybind_manager.h"
#include <string>
#include <map>
#include <unordered_map>
#include <vector>
#include <filesystem>

struct SettingsSection {
    std::vector<std::pair<std::string, std::string>> keyValues;
    std::unordered_map<std::string, size_t> keyToLineIndex;
};

class IConfigService {
public:
    virtual ~IConfigService() = default;
    virtual const Settings& getSettings() const = 0;
    virtual bool isConfigValid() const = 0;
    virtual void loadConfig() = 0;
    virtual void saveConfig(const std::map<std::string, SettingsSection>& iniData) = 0;
    virtual const std::map<std::string, SettingsSection>& getIniData() const = 0;
    virtual void setIniData(const std::map<std::string, SettingsSection>& iniData) = 0; // New method
    virtual KeybindManager& getKeybindManager() = 0;
};

class ConfigService : public IConfigService {
public:
    ConfigService(const std::string& configPath);
    const Settings& getSettings() const override { return settings_; }
    bool isConfigValid() const override;
    void loadConfig() override;
    void saveConfig(const std::map<std::string, SettingsSection>& iniData) override;
    const std::map<std::string, SettingsSection>& getIniData() const override { return iniData_; }
    void setIniData(const std::map<std::string, SettingsSection>& iniData) override; // New method
    KeybindManager& getKeybindManager() override { return keybindManager_; }

private:
    std::string configPath_;
    Settings settings_;
    KeybindManager keybindManager_;
    std::map<std::string, SettingsSection> iniData_;
    std::vector<std::string> originalLines_;

    void parseIniFile();
    void writeIniFile(const std::map<std::string, SettingsSection>& iniData);
    void setDefaultSettings();
};

#endif // CONFIG_SERVICE_H