#ifndef PLAYFIELD_OVERLAY_H
#define PLAYFIELD_OVERLAY_H

#include "imgui.h" // For ImGui types and functions
#include "render/table_data.h" // For TableData struct
#include "config/iconfig_service.h" // For IConfigService
#include "core/iwindow_manager.h" // To get playfield window dimensions
#include "render/iasset_manager.h" // For potential asset loading in metadata panel

#include <vector>
#include <string>

class PlayfieldOverlay {
public:
    // Constructor: Takes necessary dependencies from App
    PlayfieldOverlay(const std::vector<TableData>* tables, size_t* currentIndex,
                     IConfigService* configService, IWindowManager* windowManager,
                     IAssetManager* assetManager);

    // The main method to render all overlay ImGui elements
    void render();

    // Method to check if metadata panel is visible
    bool isMetadataPanelVisible() const { return showMetadataPanel_; }
    void updateSettings(const Settings& settings);

private:
    // Dependencies
    const std::vector<TableData>* tables_; // Pointer to the main list of tables
    size_t* currentIndex_; // Pointer to the current table index
    [[maybe_unused]] IConfigService* configService_; // For accessing settings
    IWindowManager* windowManager_; // To get playfield window dimensions for positioning
    [[maybe_unused]] IAssetManager* assetManager_; // For any asset needs within the overlay (e.g., metadata images)

    // UI State
    bool showMetadataPanel_ = false; // Controls visibility of the metadata panel

    // Private helper methods for drawing individual components
    void renderScrollbar();
    void renderMetadataPanel();
};

#endif // PLAYFIELD_OVERLAY_H