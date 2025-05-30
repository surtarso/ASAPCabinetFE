/**
 * @file config_state.h
 * @brief Defines the ConfigUIState class for managing configuration UI state in ASAPCabinetFE.
 *
 * This header provides the ConfigUIState class, which tracks the state of the
 * configuration UI, including the current section, changes to INI data, and UI
 * settings. It integrates with IConfigService to access and compare configuration data.
 */

#ifndef CONFIG_STATE_H
#define CONFIG_STATE_H

#include <vector>
#include <string>
#include <map>
#include "config/settings_section.h"
#include "config/iconfig_service.h"

/**
 * @class ConfigUIState
 * @brief Manages the state of the configuration UI.
 *
 * This class tracks the current section, changes to INI data, and UI settings such
 * as color picker visibility and save message timers. It provides methods to detect
 * changes in specific configuration sections or settings, used by ConfigUI to
 * coordinate UI updates and save operations.
 */
class ConfigUIState {
public:
    /**
     * @brief Constructs a ConfigUIState instance.
     *
     * Initializes the state with a configuration service for accessing INI data.
     *
     * @param configService The configuration service for settings access.
     */
    ConfigUIState(IConfigService* configService);

    std::string currentSection;                          ///< Name of the currently selected INI section.
    bool hasChanges = false;                            ///< Flag indicating if INI data has changed.
    float saveMessageTimer = 0.0f;                      ///< Timer for displaying save confirmation messages.
    std::map<std::string, bool> showPicker;             ///< Map of keys to color picker visibility flags.
    std::map<std::string, SettingsSection> lastSavedIniData; ///< Last saved INI data for change detection.

    /**
     * @brief Checks if a section’s INI data has changed.
     *
     * Compares the current INI data for the specified section with the last saved data.
     *
     * @param sectionName The name of the INI section to check.
     * @param currentIniData The current INI data.
     * @return True if the section has changed, false otherwise.
     */
    bool hasSectionChanged(const std::string& sectionName, const std::map<std::string, SettingsSection>& currentIniData) const;

    /**
     * @brief Checks if window settings have changed.
     *
     * Compares window-related settings (e.g., position, size) in the current INI data
     * with the last saved data.
     *
     * @param currentIniData The current INI data.
     * @return True if window settings have changed, false otherwise.
     */
    bool hasWindowSettingsChanged(const std::map<std::string, SettingsSection>& currentIniData) const;

    /**
     * @brief Checks if visibility settings have changed.
     *
     * Compares visibility-related settings (e.g., show/hide flags) in the current INI
     * data with the last saved data.
     *
     * @param currentIniData The current INI data.
     * @return True if visibility settings have changed, false otherwise.
     */
    bool hasVisibilitySettingsChanged(const std::map<std::string, SettingsSection>& currentIniData) const;

    /**
     * @brief Checks if font settings have changed.
     *
     * Compares font-related settings (e.g., font path, size) in the current INI data
     * with the last saved data.
     *
     * @param currentIniData The current INI data.
     * @return True if font settings have changed, false otherwise.
     */
    bool hasFontSettingsChanged(const std::map<std::string, SettingsSection>& currentIniData) const;

    /**
     * @brief Checks if the title data source has changed.
     *
     * Compares the title data source setting in the current INI data with the last
     * saved data.
     *
     * @param currentIniData The current INI data.
     * @return True if the title data source has changed, false otherwise.
     */
    bool hasTitleDataChanged(const std::map<std::string, SettingsSection>& currentIniData) const;

    /**
     * @brief Checks if the video backend has changed.
     *
     * Compares the video backend setting in the current INI data with the last saved data.
     *
     * @param currentIniData The current INI data.
     * @return True if the video backend has changed, false otherwise.
     */
    bool hasVideoBackendChanged(const std::map<std::string, SettingsSection>& currentIniData) const;

    /**
     * @brief Checks if the force images only setting has changed.
     *
     * Compares the force images only setting in the provided INI data with the last
     * saved data.
     *
     * @param oldIniData The INI data to compare against (typically last saved).
     * @return True if the force images only setting has changed, false otherwise.
     */
    bool hasForceImagesOnlyChanged(const std::map<std::string, SettingsSection>& oldIniData) const;

    /**
     * @brief Checks if metadata settings have changed.
     *
     * Compares metadata-related settings in the current INI data with the last saved data.
     *
     * @param currentIniData The current INI data.
     * @return True if metadata settings have changed, false otherwise.
     */
    bool hasMetadataSettingsChanged(const std::map<std::string, SettingsSection>& currentIniData) const;

private:
    static const std::vector<std::string> sectionOrder_; ///< Ordered list of INI section names.
    IConfigService* configService_;                      ///< Configuration service for settings access.
};

#endif // CONFIG_STATE_H