#ifndef CONFIG_FILE_HANDLER_H
#define CONFIG_FILE_HANDLER_H

#include "config/settings_section.h"
#include <map>
#include <string>
#include <vector>

class ConfigFileHandler {
public:
    ConfigFileHandler(const std::string& configPath);
    std::map<std::string, SettingsSection> readConfig(std::vector<std::string>& originalLines);
    void writeConfig(const std::map<std::string, SettingsSection>& iniData);

private:
    std::string configPath_;
};

#endif // CONFIG_FILE_HANDLER_H