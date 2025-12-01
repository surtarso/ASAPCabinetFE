/**
 * @file table_override_editor.h
 * @brief Defines the TableOverrideEditor class for editing table overrides in ASAPCabinetFE.
 *
 * This header provides the TableOverrideEditor class, which renders an ImGui panel for editing
 * overrideable TableData fields (e.g., title, playfieldVideo). The panel supports saving changes
 * to <table_name>.json via TableOverrideManager, discarding changes, or resetting to original
 * values from asapcab_index.json. It is designed to be separate from ConfigUI and uses keyboard/mouse
 * input during setup mode.
 */

#ifndef TABLE_OVERRIDE_EDITOR_H
#define TABLE_OVERRIDE_EDITOR_H

#include "data/table_data.h"
#include "data/asapcab/table_override_manager.h"
#include <map>
#include <string>

class TableOverrideEditor {
public:
    /**
     * @brief Constructs a TableOverrideEditor for a specific table.
     *
     * Initializes the editor with the table's current TableData values, prefilled in text fields.
     *
     * @param table The TableData object to edit.
     * @param overrideManager The TableOverrideManager for saving overrides.
     */
    TableOverrideEditor(TableData& table, TableOverrideManager& overrideManager);

    /**
     * @brief Renders the ImGui panel for editing overrides.
     *
     * Displays text fields for overrideable fields, prefilled with current values, and buttons
     * for Save, Discard, and Reset. Returns false if the panel should close (e.g., Discard pressed).
     *
     * @return True if the panel should remain open, false if it should close.
     */
    bool render();

    /**
     * @brief Checks if changes were saved.
     *
     * @return True if Save was clicked, false otherwise (e.g., Discard or Reset).
     */
    bool wasSaved() const { return saved_; }

private:
    /**
     * @brief Saves edited fields to <table_name>.json.
     *
     * Calls TableOverrideManager::saveOverride with changed fields only.
     */
    void save();

    /**
     * @brief Resets fields to original TableData values.
     *
     * Restores values from asapcab_index.json or loaded TableData.
     */
    // void reset();

    TableData& table_; ///< Reference to the TableData being edited.
    TableOverrideManager& overrideManager_; ///< Manager for saving overrides.
    std::map<std::string, std::string> fields_; ///< Current field values.
    std::map<std::string, std::string> originalFields_; ///< Original field values for reset.
    bool shouldClose_; ///< Flag to close the panel (e.g., on Discard).
    bool saved_;
};

#endif // TABLE_OVERRIDE_EDITOR_H
