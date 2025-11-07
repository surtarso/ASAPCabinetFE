#ifndef BUTTON_ACTIONS_H
#define BUTTON_ACTIONS_H

#include <string>
#include "config/iconfig_service.h" // for IConfigService interface

/**
 * @brief ButtonActions replicates simple table management operations (e.g. open folder, extract VBS)
 *        similar to vpxguitools/TableActions but scoped for EditorUI.
 */
class ButtonActions {
public:
    explicit ButtonActions(IConfigService* config);

    /**
     * @brief Opens the containing folder for the given file path.
     * @param filepath Full path to a VPX table or other asset file.
     */
    void openFolder(const std::string& filepath);

private:
    IConfigService* config_; // Non-owning pointer
};

#endif // BUTTON_ACTIONS_H
