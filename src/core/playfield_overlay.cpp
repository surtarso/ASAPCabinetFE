#include "core/playfield_overlay.h"
#include "utils/logging.h" // For LOG_INFO, LOG_DEBUG, etc.

// --- Configuration for UI elements ---
const float SCROLLBAR_WIDTH = 12.0f;        // Width of the scrollbar bar and thumb
const float SCROLLBAR_PADDING = 15.0f;      // Padding from the edge of the screen
const float THUMB_MIN_HEIGHT = 20.0f;       // Minimum height for the scrollbar thumb
const float METADATA_PANEL_WIDTH_FACTOR = 0.7f;
const float METADATA_PANEL_HEIGHT_FACTOR = 0.5f; // Metadata panel takes 40% of playfield height
const float METADATA_PANEL_ALPHA = 0.6f;    // Transparency for the metadata panel background

PlayfieldOverlay::PlayfieldOverlay(const std::vector<TableData>* tables, size_t* currentIndex,
                                   IConfigService* configService, IWindowManager* windowManager,
                                   IAssetManager* assetManager)
    : tables_(tables), currentIndex_(currentIndex), configService_(configService),
      windowManager_(windowManager), assetManager_(assetManager),
      showMetadataPanel_(configService->getSettings().showMetadata) // can be toggled by user input
{
    LOG_INFO("PlayfieldOverlay: Initialized.");
}

void PlayfieldOverlay::updateSettings(const Settings& settings) {
    bool oldShowMetadataPanel = showMetadataPanel_;
    showMetadataPanel_ = settings.showMetadata;
    LOG_DEBUG("PlayfieldOverlay: Updated showMetadataPanel_ from " << (oldShowMetadataPanel ? "true" : "false")
              << " to " << (showMetadataPanel_ ? "true" : "false"));
}

void PlayfieldOverlay::render() {
    int playfieldWidth = 0;
    int playfieldHeight = 0;
    SDL_Window* playfieldWindow = windowManager_->getPlayfieldWindow();
    if (playfieldWindow) {
        SDL_GetWindowSize(playfieldWindow, &playfieldWidth, &playfieldHeight);
    } else {
        LOG_ERROR("PlayfieldOverlay: Playfield window is null, cannot get dimensions.");
        return; // Exit render if main window is not available
    }

    // Set the position and size of the main overlay window
    ImGui::SetNextWindowPos(ImVec2(0, 0)); // Top-left corner
    ImGui::SetNextWindowSize(ImVec2(playfieldWidth, playfieldHeight)); // Span the entire playfield
    ImGui::Begin("Playfield Overlay", nullptr,
        ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoBackground |
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNav |
        ImGuiWindowFlags_NoInputs
    );

    // Render Scrollbar (centered at the top)
    float scrollbarLength = playfieldWidth * 0.5f; // 50% of playfield width
    float posX = (playfieldWidth - scrollbarLength) / 2.0f; // Center horizontally
    ImGui::SetCursorPos(ImVec2(posX, SCROLLBAR_PADDING));
    renderScrollbar();

    // Render Metadata Panel (positioned at the bottom-left)
    if (showMetadataPanel_) {
        renderMetadataPanel();
    }

    ImGui::End(); // End the main overlay window
}

void PlayfieldOverlay::renderScrollbar() {
    if (!tables_ || tables_->empty() || tables_->size() <= 1) {
        return; // No scrollbar needed for 0 or 1 table
    }

    size_t numTables = tables_->size();

    // The width of the scrollbar is 50% of the playfield width
    float playfieldWidth = ImGui::GetWindowWidth();
    const float SCROLLBAR_LENGTH = playfieldWidth * 0.5f;
    float thumbWidth = THUMB_MIN_HEIGHT; // Use same min size for consistency

    float scrollProgress = (float)*currentIndex_ / (numTables - 1); // Normalized position (0.0 to 1.0)
    float thumbXOffset = scrollProgress * (SCROLLBAR_LENGTH - thumbWidth);

    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ImVec2 cursorScreenPos = ImGui::GetCursorScreenPos(); // Top-left of scrollbar

    // Draw the scrollbar background
    ImVec2 bgMin = cursorScreenPos;
    ImVec2 bgMax = ImVec2(bgMin.x + SCROLLBAR_LENGTH, bgMin.y + SCROLLBAR_WIDTH);
    draw_list->AddRectFilled(bgMin, bgMax, IM_COL32(50, 50, 50, 200), SCROLLBAR_WIDTH * 0.5f); // Dark gray, rounded

    // Draw the scrollbar thumb
    ImVec2 thumbMin = ImVec2(bgMin.x + thumbXOffset, bgMin.y);
    ImVec2 thumbMax = ImVec2(bgMin.x + thumbXOffset + thumbWidth, bgMin.y + SCROLLBAR_WIDTH);
    draw_list->AddRectFilled(thumbMin, thumbMax, IM_COL32(150, 150, 150, 255), SCROLLBAR_WIDTH * 0.5f); // Lighter gray, rounded

    // Advance cursor for next ImGui elements
    ImGui::Dummy(ImVec2(SCROLLBAR_LENGTH, SCROLLBAR_WIDTH + SCROLLBAR_PADDING));
}

void PlayfieldOverlay::renderMetadataPanel() {
    if (!tables_ || tables_->empty() || *currentIndex_ >= tables_->size()) {
        return; // No table data to display
    }

    const TableData& currentTable = tables_->at(*currentIndex_);

    // Get the dimensions of the parent "Playfield Overlay" window
    float playfieldWidth = ImGui::GetWindowWidth();
    float playfieldHeight = ImGui::GetWindowHeight();

    // Calculate the metadata panel's dimensions based on factors
    float panelWidth = playfieldWidth * METADATA_PANEL_WIDTH_FACTOR;
    float panelHeight = playfieldHeight * METADATA_PANEL_HEIGHT_FACTOR;

    // Calculate the position to center the panel
    float posX = (playfieldWidth - panelWidth) / 2.0f;
    float posY = (playfieldHeight - panelHeight) / 2.0f;

    ImGui::SetNextWindowPos(ImVec2(posX, posY));
    ImGui::SetNextWindowSize(ImVec2(panelWidth, panelHeight));
    ImGui::SetNextWindowBgAlpha(METADATA_PANEL_ALPHA);

    // Begin the metadata panel window without title bar or close button
    if (ImGui::Begin("Table Metadata", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "METADATA INFO"); // Yellow header
        std::filesystem::path filePath(currentTable.vpxFile);
        ImGui::Text("File: %s", filePath.filename().string().c_str());
        ImGui::Text("Table Name: %s", currentTable.tableName.c_str());
        ImGui::Text("Author: %s", currentTable.authorName.c_str());
        ImGui::Text("Rom Name: %s", currentTable.gameName.c_str());
        ImGui::Text("Manufacturer: %s", currentTable.manufacturer.c_str());
        ImGui::Text("Year: %s", currentTable.year.c_str());
        ImGui::Text("Release Date: %s", currentTable.releaseDate.c_str());
        ImGui::Text("Version: %s", currentTable.tableVersion.c_str());
        ImGui::Text("Revision: %s", currentTable.tableRevision.c_str());
        ImGui::Text("Save Date: %s", currentTable.tableSaveDate.c_str());
        ImGui::Text("Last Modified: %s", currentTable.lastModified.c_str());
        
        ImGui::Separator();
        ImGui::TextWrapped("Description: %s", currentTable.tableDescription.c_str());
        
        ImGui::End(); // End the metadata panel window
    }
}
