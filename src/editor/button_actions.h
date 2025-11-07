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
     * @brief Extracts the VBScript from a .vpx file using vpxtool.
     * @param filepath Full path to the .vpx table.
     */
    void extractVBS(const std::string& filepath); // Ported from old TableActions

    /**
     * @brief Opens a file (like a .vbs script) in the user's default external editor.
     * @param filepath Full path to the file to open.
     * @return True if the command succeeded.
     */
    bool openInExternalEditor(const std::string& filepath); // Ported from old TableActions

    /**
     * @brief Opens the containing folder for the given file path.
     * @param filepath Full path to a VPX table or other asset file.
     */
    void openFolder(const std::string& filepath);

private:
    IConfigService* config_; // Non-owning pointer
};

#endif // BUTTON_ACTIONS_H
