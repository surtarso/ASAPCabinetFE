#ifndef FOOTER_ACTIONS_H
#define FOOTER_ACTIONS_H

#include <string>
#include <vector>
#include <functional>
#include "config/iconfig_service.h" // for IConfigService interface
#include "launcher/itable_launcher.h"
#include "data/table_data.h"
#include "tables/itable_callbacks.h"  // to call save() after updating stats

/**
 * @brief Handles simple table-related operations for EditorUI,
 *        similar to vpxguitools/TableActions.
 */
class ButtonActions {
public:
    // explicit ButtonActions(IConfigService* config);
    explicit ButtonActions(IConfigService* config, ITableCallbacks* tableCallbacks);

    /**
     * @brief Extracts the VBScript from a .vpx file using vpxtool.
     * @param filepath Full path to the .vpx table.
     */
    void extractOrOpenVbs(
        const std::string& filepath,
        std::function<void(const std::string&)> onOutput,
        std::function<void()> onFinished
    );


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

    void handleRowNavigation(int& selectedRow, int totalRows);

    /**
     * @brief Launches the selected table and updates its statistics (play count, time, isBroken).
     * @param selectedTable The TableData object selected (used for its vpxFile key).
     * @param masterTables The mutable reference to the master list of tables for updating stats.
     * @param launcher The table launcher dependency.
     * @param refreshUICallback Function to call after stats are updated to refresh the UI view.
     */
    void launchTableWithStats(
        const TableData& selectedTable,
        std::vector<TableData>& masterTables,
        ITableLauncher* launcher,
        std::function<void()> refreshUICallback);

private:
    IConfigService* config_; // Non-owning pointer
    ITableCallbacks* tableCallbacks_; // Non-owning pointer to save updated stats
    bool pendingSearchFocus_ = false; // Deferred focus flag
};

#endif // FOOTER_ACTIONS_H
