#ifndef CONFIG_SERVICE_H
#define CONFIG_SERVICE_H

#include "utils/vpinballx_ini_reader.h"
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

    void updateWindowSetup(int& playfieldX, int& playfieldY, int& playfieldWidth, int& playfieldHeight,
                            int& backglassX, int& backglassY, int& backglassWidth, int& backglassHeight,
                            int& dmdX, int& dmdY, int& dmdWidth, int& dmdHeight,
                            int& topperX, int& topperY, int& topperWidth, int& topperHeight) override;

private:
    std::string configPath_;
    Settings settings_;
    KeybindManager keybindManager_;
    nlohmann::json jsonData_;

    void applyPostProcessing();
    void applyIniSettings(const std::optional<VPinballXIniSettings>& iniSettings);
    void updateJsonWithIniValues(const std::optional<VPinballXIniSettings>& iniSettings);
};

#endif // CONFIG_SERVICE_H