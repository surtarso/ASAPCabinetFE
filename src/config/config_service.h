#ifndef CONFIG_SERVICE_H
#define CONFIG_SERVICE_H

#include "iconfig_service.h"
#include <json.hpp>
#include <string>

class ConfigService : public IConfigService {
public:
    ConfigService(const std::string& configPath);
    ~ConfigService() override = default;

    const Settings& getSettings() const override;
    bool isConfigValid() const override;
    void loadConfig() override;
    void saveConfig() override;
    KeybindManager& getKeybindManager() override;

private:
    std::string configPath_;
    Settings settings_;
    KeybindManager keybindManager_;
    nlohmann::json jsonData_;

    void applyPostProcessing();
};

#endif // CONFIG_SERVICE_H