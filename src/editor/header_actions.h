#ifndef EDITOR_HEADER_ACTIONS_H
#define EDITOR_HEADER_ACTIONS_H

#include <string>
#include <filesystem>
#include <imgui.h>
#include "editor/ui/editor_ui.h"  // for EditorUI reference
#include "log/logging.h"

/**
 * @brief Handles all "Advanced Menu" operations from the editor header.
 *
 * This includes file management (delete, compress, export, repair) and system tools.
 * Each action has its own confirmation popup and uses the existing logging + rescan pipeline.
 */
namespace header_actions {

    /**
     * @brief Draws all modal popups (confirmations, progress, etc.) triggered by Advanced Menu actions.
     * Should be called once per frame after the Advanced menu is rendered.
     */
    void drawModals(EditorUI& ui);

    /**
     * @brief Initiates delete folder action for currently selected table.
     * Opens a confirmation modal if valid.
     */
    void requestDeleteTableFolder(EditorUI& ui);

    /**
     * @brief Initiates delete of an individual file (INI, VBS, Metadata, Override).
     * @param ui Reference to EditorUI.
     * @param fileType Logical file identifier (e.g., "ini", "vbs", "metadata", "override").
     */
    void requestDeleteTableFile(EditorUI& ui, const std::string& fileType);

    /**
     * @brief Performs a compression action (e.g., zipping the table folder).
     */
    void requestCompressTableFolder(EditorUI& ui);

    /**
     * @brief Runs external VPXTool options, if configured.
     */
    void vpxtoolRun(EditorUI& ui, const std::string& command);

    /**
     * @brief Clears all caches (textures, metadata, temp, etc.)
     */
    void clearAllCaches(EditorUI& ui);

}  // namespace header_actions

#endif // EDITOR_HEADER_ACTIONS_H
