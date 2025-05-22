#ifndef ICONFIG_SERVICE_H
#define ICONFIG_SERVICE_H

#include <filesystem>
#include "config/settings.h"  // For Settings
#include "keybinds/keybind_manager.h"  // For KeybindManager
#include <map>
#include <string>

struct SettingsSection;  // Forward declare to avoid including full definition

/**
 * @brief Interface for configuration services.
 *
 * The IConfigService interface declares a contract for managing application configurations.
 * It provides methods for retrieving settings, validating configuration, loading and saving configuration data,
 * managing INI file data, and updating window positions for various display areas.
 *
 * The interface includes the following functionalities:
 * - Retrieve the current settings via getSettings().
 * - Validate the configuration state via isConfigValid().
 * - Load the configuration data via loadConfig().
 * - Save the configuration to persistent storage via saveConfig().
 * - Access and modify INI file data via getIniData() and setIniData().
 * - Obtain a reference to the key binding manager via getKeybindManager().
 * - Update the positions of specific application windows (playfield, backglass, and DMD) via updateWindowPositions().
 */
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
    virtual void updateWindowPositions(int playfieldX, int playfieldY, int backglassX, int backglassY, int dmdX, int dmdY) = 0;
};

#endif // ICONFIG_SERVICE_H
