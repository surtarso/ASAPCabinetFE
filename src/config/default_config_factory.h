#ifndef DEFAULT_CONFIG_FACTORY_H
#define DEFAULT_CONFIG_FACTORY_H

#include "config/settings.h"
#include "config/settings_section.h"
#include "config/config_schema.h"
#include <map>
#include <string>

class DefaultConfigFactory {
public:
    DefaultConfigFactory();
    std::map<std::string, SettingsSection> getDefaultIniData() const;
    void getDefaultSettings(Settings& settings) const;

private:
    ConfigSchema schema_;
};

#endif // DEFAULT_CONFIG_FACTORY_H