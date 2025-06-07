#ifndef ICONFIG_SERVICE_H
#define ICONFIG_SERVICE_H

#include <filesystem>
#include "settings.h"
#include "keybinds/keybind_manager.h"
#include <string>

/**
 * @class IConfigService
 * @brief Interface for managing application configuration and keybindings in ASAPCabinetFE.
 *
 * This pure virtual class defines methods for retrieving and modifying application
 * settings, validating configuration, loading and saving JSON-based configuration data,
 * managing keybindings via KeybindManager, and updating window positions for playfield,
 * backglass, DMD, and topper displays. Implementers handle JSON configuration file operations.
 */
class IConfigService {
public:
    /**
     * @brief Virtual destructor for proper cleanup of derived classes.
     */
    virtual ~IConfigService() = default;

    /**
     * @brief Gets the current application settings.
     *
     * @return A const reference to the Settings object.
     */
    virtual const Settings& getSettings() const = 0;

    /**
     * @brief Checks if the configuration is valid.
     *
     * @return True if the configuration is valid, false otherwise.
     */
    virtual bool isConfigValid() const = 0;

    /**
     * @brief Loads configuration data from the JSON file.
     */
    virtual void loadConfig() = 0;

    /**
     * @brief Saves the current configuration data to the JSON file.
     */
    virtual void saveConfig() = 0;

    /**
     * @brief Gets the keybinding manager.
     *
     * @return A reference to the KeybindManager object.
     */
    virtual KeybindManager& getKeybindManager() = 0;
};

#endif // ICONFIG_SERVICE_H