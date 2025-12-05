#ifndef TABLE_OVERRIDE_EDITOR_H
#define TABLE_OVERRIDE_EDITOR_H
#include "data/asapcab/table_override_manager.h"
#include "data/table_data.h"
#include <map>
#include <string>
/**

@class TableOverrideEditor
@brief Manages the editing of table overrides for display, sorting, and VPSDB matching.

This class renders an ImGui panel allowing users to override specific TableData fields:

bestTitle: Custom title for frontend display and sorting.

bestManufacturer: Custom manufacturer for display and sorting.

bestYear: Custom year for display and sorting.

vpsId: Manual VPSDB ID for accurate matchmaking during scans.

Changes are saved to a per-table JSON file via TableOverrideManager. The panel includes
explanatory text for user feedback and supports saving, discarding changes.
*/
class TableOverrideEditor
{
public:
    /*
    @brief Constructs a TableOverrideEditor for a specific table.

    Initializes the editor with the table's current overrideable values prefilled in text fields.

    @param table Reference to the TableData object to edit.
    @param overrideManager Reference to the TableOverrideManager for saving overrides.
    */
    TableOverrideEditor(TableData &table, TableOverrideManager &overrideManager);
    /**
    @brief Renders the ImGui panel for editing overrides.

    Displays explanatory text, text fields for overrideable fields, and buttons for Save and Discard.
    The panel is centered, unmovable, and unresizable. Returns false if the panel should close.

    @return True if the panel should remain open, false if it should close (e.g., Discard or Save pressed).
    */
    bool render();

private:
    /**

    @brief Saves edited fields to the per-table override JSON file.

    Only changed fields are saved via TableOverrideManager::saveOverride.
    */
    void save();

    TableData &table_;                                  ///< Reference to the TableData being edited.
    TableOverrideManager &overrideManager_;             ///< Manager for saving overrides.
    std::map<std::string, std::string> fields_;         ///< Current field values.
    std::map<std::string, std::string> originalFields_; ///< Original field values for change detection.
    bool shouldClose_;                                  ///< Flag to close the panel (e.g., on Discard or Save).
    bool saved_;                                        ///< Flag indicating if changes were saved.
};
#endif // TABLE_OVERRIDE_EDITOR_H
