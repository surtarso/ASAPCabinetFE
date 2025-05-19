#ifndef SETTINGS_PARSER_H
#define SETTINGS_PARSER_H

#include "config/settings.h"
#include "config/settings_section.h"
#include "keybinds/keybind_manager.h"
#include <map>
#include <string>

class SettingsParser {
public:
    SettingsParser(const std::string& configPath);
    void parse(const std::map<std::string, SettingsSection>& iniData, Settings& settings, KeybindManager& keybindManager) const;

private:
    std::string configPath_;

    void setDefaultSettings(Settings& settings) const;
};

#endif // SETTINGS_PARSER_H