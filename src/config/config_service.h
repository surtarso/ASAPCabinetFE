#ifndef CONFIG_SERVICE_H
#define CONFIG_SERVICE_H

#include "config/iconfig_service.h"
#include "config/settings.h"
#include "config/config_file_handler.h"
#include "config/settings_section.h"
#include "config/settings_parser.h"
#include "keybinds/keybind_manager.h"
#include <map>
#include <string>
#include <vector>

class ConfigService : public IConfigService {
public:
    ConfigService(const std::string& configPath);
    const Settings& getSettings() const override { return settings_; }
    bool isConfigValid() const override;
    void loadConfig() override;
    void saveConfig(const std::map<std::string, SettingsSection>& iniData) override;
    const std::map<std::string, SettingsSection>& getIniData() const override { return iniData_; }
    void setIniData(const std::map<std::string, SettingsSection>& iniData) override;
    KeybindManager& getKeybindManager() override { return keybindManager_; }
    void updateWindowPositions(int playfieldX, int playfieldY, int backglassX, int backglassY, int dmdX, int dmdY) override;

private:
    std::string configPath_;
    Settings settings_;
    KeybindManager keybindManager_;
    std::map<std::string, SettingsSection> iniData_;
    std::vector<std::string> originalLines_;
    ConfigFileHandler fileHandler_;
    SettingsParser parser_;

    void setDefaultSettings();
    void initializeIniData();
};

#endif // CONFIG_SERVICE_H