#ifndef PLAYFIELD_OVERLAY_H
#define PLAYFIELD_OVERLAY_H

#include "imgui.h"
#include "data/table_data.h"
#include "config/iconfig_service.h"
#include "core/iwindow_manager.h"
#include "render/iasset_manager.h"

#include <vector>
#include <string>

/**
 * @class PlayfieldOverlay
 * @brief Manages ImGui-based UI overlays for the playfield display.
 *
 * This class renders ImGui elements, such as scrollbars, navigation arrows, and metadata
 * panels, on the playfield display. It uses table data to display metadata, configuration
 * services for settings, window management for positioning, and asset management for
 * potential texture or video needs. Customization is supported through namespace-defined
 * settings (e.g., NavigationArrowSettings for arrows, ScrollbarSettings for scrollbars,
 * MetadataPanelSettings for panels) that can be adjusted via configUI.
 */
class PlayfieldOverlay {
public:
    /**
     * @brief Constructs a PlayfieldOverlay instance.
     *
     * Initializes the overlay with dependencies for table data, current table index,
     * configuration, window management, and asset management. The overlay's appearance
     * is governed by customizable settings in namespaces (e.g., NavigationArrowSettings,
     * ScrollbarSettings, MetadataPanelSettings) that can be modified via configUI.
     *
     * @param tables Pointer to the list of table data.
     * @param currentIndex Pointer Items the current table index.
     * @param configService The configuration service for accessing settings.
     * @param windowManager The window manager for playfield window dimensions.
     * @param assetManager The asset manager for accessing textures or video players.
     */
    PlayfieldOverlay(const std::vector<TableData>* tables, size_t* currentIndex,
                     IConfigService* configService, IWindowManager* windowManager,
                     IAssetManager* assetManager, std::function<void()> refreshUICallback);

    /**
     * @brief Renders all ImGui overlay elements.
     *
     * Draws the scrollbar, navigation arrows, and metadata panel (if visible) on the
     * playfield display using ImGui. The appearance and visibility of these elements
     * are controlled by settings in NavigationArrowSettings, ScrollbarSettings, and
     * MetadataPanelSettings, which can be tweaked via configUI.
     */
    void render();

    /**
     * @brief Checks if the metadata panel is visible.
     *
     * @return True if the metadata panel is visible, false otherwise.
     * @note Controlled by the showMetadataPanel_ member, which is updated via configUI.
     */
    bool isMetadataPanelVisible() const { return showMetadataPanel_; }

    /**
     * @brief Updates overlay settings.
     *
     * Reconfigures the overlay based on the provided application settings, such as
     * UI visibility or positioning. This includes updating the showMetadataPanel_
     * flag and can be extended to reflect changes in NavigationArrowSettings,
     * ScrollbarSettings, or MetadataPanelSettings via configUI.
     *
     * @param settings The application settings to apply.
     */
    void updateSettings(const Settings& settings);

    void ResetMetadataFlags();

private:
    const std::vector<TableData>* tables_; ///< Pointer to the list of table data used for metadata display.
    size_t* currentIndex_;                 ///< Pointer to the current table index for navigation and scrollbar positioning.
    [[maybe_unused]] IConfigService* configService_;        ///< Configuration service for accessing and updating settings, including UI visibility.
    IWindowManager* windowManager_;        ///< Window manager for retrieving playfield window dimensions and positioning overlays.
    [[maybe_unused]] IAssetManager* assetManager_;          ///< Asset manager for potential texture or video assets in future enhancements.
    bool showMetadataPanel_;               ///< Flag controlling metadata panel visibility, set via configService and updated by updateSettings.
    bool resetMetadataFlags_ = false;
    std::function<void()> refreshUICallback_;

    /**
     * @brief Renders the scrollbar UI element.
     *
     * Draws the scrollbar for navigating the table list using ImGui. The scrollbar's
     * appearance (e.g., width, padding, thumb size, colors) and visibility are defined
     * in ScrollbarSettings, allowing customization via configUI.
     */
    void renderScrollbar();
};

#endif // PLAYFIELD_OVERLAY_H
