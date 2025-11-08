#ifndef BUTTON_ACTIONS_H
#define BUTTON_ACTIONS_H

#include <string>
#include "config/iconfig_service.h" // for IConfigService interface

/**
 * @brief Handles simple table-related operations for EditorUI,
 *        similar to vpxguitools/TableActions.
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

    /**
     * @brief Detects if the user started typing alphanumeric keys and auto-focuses the search input.
     * @param searchInputId The ImGui ID of the search field (e.g. "##SearchInputTop").
     */
    void handleKeyboardSearchFocus(char* searchBuffer,
    std::string& searchQuery,
    std::function<void()> filterAndSort,
    std::function<void()> onEnter);

private:
    IConfigService* config_; // Non-owning pointer
    bool pendingSearchFocus_ = false; // Deferred focus flag
};

#endif // BUTTON_ACTIONS_H
