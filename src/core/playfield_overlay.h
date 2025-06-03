/**
 * @file playfield_overlay.h
 * @brief Defines the PlayfieldOverlay class for rendering ImGui overlays in ASAPCabinetFE.
 *
 * This header provides the PlayfieldOverlay class, which manages ImGui-based UI elements
 * for the playfield display, including scrollbars and metadata panels. It integrates with
 * table data, configuration services, window management, and asset management to render
 * interactive overlays.
 */

#ifndef PLAYFIELD_OVERLAY_H
#define PLAYFIELD_OVERLAY_H

#include "imgui.h"
#include "tables/table_data.h"
#include "config/iconfig_service.h"
#include "core/iwindow_manager.h"
#include "render/iasset_manager.h"

#include <vector>
#include <string>

/**
 * @class PlayfieldOverlay
 * @brief Manages ImGui-based UI overlays for the playfield display.
 *
 * This class renders ImGui elements, such as scrollbars and metadata panels, on the
 * playfield display. It uses table data to display metadata, configuration services
 * for settings, window management for positioning, and asset management for potential
 * texture or video needs.
 */
class PlayfieldOverlay {
public:
    /**
     * @brief Constructs a PlayfieldOverlay instance.
     *
     * Initializes the overlay with dependencies for table data, current table index,
     * configuration, window management, and asset management.
     *
     * @param tables Pointer to the list of table data.
     * @param currentIndex Pointer to the current table index.
     * @param configService The configuration service for accessing settings.
     * @param windowManager The window manager for playfield window dimensions.
     * @param assetManager The asset manager for accessing textures or video players.
     */
    PlayfieldOverlay(const std::vector<TableData>* tables, size_t* currentIndex,
                     IConfigService* configService, IWindowManager* windowManager,
                     IAssetManager* assetManager);

    /**
     * @brief Renders all ImGui overlay elements.
     *
     * Draws the scrollbar and metadata panel (if visible) on the playfield display
     * using ImGui, based on the current table index and settings.
     */
    void render();

    /**
     * @brief Checks if the metadata panel is visible.
     *
     * @return True if the metadata panel is visible, false otherwise.
     */
    bool isMetadataPanelVisible() const { return showMetadataPanel_; }

    /**
     * @brief Updates overlay settings.
     *
     * Reconfigures the overlay based on the provided application settings, such as
     * UI visibility or positioning.
     *
     * @param settings The application settings to apply.
     */
    void updateSettings(const Settings& settings);

private:
    const std::vector<TableData>* tables_; ///< Pointer to the list of table data.
    size_t* currentIndex_;                 ///< Pointer to the current table index.
    [[maybe_unused]] IConfigService* configService_;        ///< Configuration service for settings.
    IWindowManager* windowManager_;        ///< Window manager for playfield dimensions.
    [[maybe_unused]] IAssetManager* assetManager_;          ///< Asset manager for textures or video players.
    bool showMetadataPanel_;               ///< Flag controlling metadata panel visibility.

    /**
     * @brief Renders the scrollbar UI element.
     *
     * Draws the scrollbar for navigating the table list using ImGui.
     */
    void renderScrollbar();

    /**
     * @brief Renders the metadata panel UI element.
     *
     * Draws the metadata panel with table information (e.g., title, author) using
     * ImGui, if the panel is visible.
     */
    void renderMetadataPanel();
};

#endif // PLAYFIELD_OVERLAY_H