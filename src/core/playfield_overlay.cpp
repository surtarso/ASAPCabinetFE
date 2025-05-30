/**
 * @file playfield_overlay.cpp
 * @brief Implements the PlayfieldOverlay class for rendering ImGui overlays in ASAPCabinetFE.
 *
 * This file provides the implementation for rendering ImGui-based UI elements, such as
scrollbars and metadata panels, on the playfield display. It integrates with table data,
configuration services, window management, and asset management.
 */

#include "core/playfield_overlay.h"
#include "utils/logging.h"
#include <SDL.h>

const float SCROLLBAR_WIDTH = 12.0f;        ///< Width of the scrollbar bar and thumb
const float SCROLLBAR_PADDING = 15.0f;     ///< Padding from the edge of the screen
const float THUMB_MIN_HEIGHT = 20.0f;    ///< Minimum height for the scrollbar thumb
const float METADATA_PANEL_WIDTH_FACTOR = 0.7f;       ///< Metadata panel takes 70% of playfield width
const float METADATA_PANEL_HEIGHT_FACTOR = 0.5f;      ///< Metadata panel takes 50% of playfield height
const float METADATA_PANEL_ALPHA = 0.6f;         ///< Transparency for the metadata panel background

PlayfieldOverlay::PlayfieldOverlay(const std::vector<TableData>* tables, size_t* currentIndex,
                            IConfigService* configService, IWindowManager* windowManager,
                            IAssetManager* assetManager)
    : tables_(tables),
      currentIndex_(currentIndex),
      configService_(configService),
      windowManager_(windowManager),
      assetManager_(assetManager),
      showMetadataPanel_(configService->getSettings().showMetadata)
{
    LOG_DEBUG("PlayfieldOverlay: Initialized.");
}

void PlayfieldOverlay::updateSettings(const Settings& settings) {
    showMetadataPanel_ = settings.showMetadata;
    LOG_DEBUG("PlayfieldOverlay: Updated showMetadataPanel to " << (showMetadataPanel_ ? "true" : "false"));
}

void PlayfieldOverlay::render() {
    int playfieldWidth = 0;
    int playfieldHeight = 0;
    SDL_Window* playfieldWindow = windowManager_->getPlayfieldWindow();
    if (playfieldWindow) {
        SDL_GetWindowSize(playfieldWindow, &playfieldWidth, &playfieldHeight);
    } else {
        LOG_ERROR("PlayfieldOverlay: Playfield window is null, cannot get dimensions.");
        return;
    }

    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2(playfieldWidth, playfieldHeight));
    ImGui::Begin("Playfield Overlay", nullptr,
        ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoBackground |
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNav |
        ImGuiWindowFlags_NoInputs
    );

    float scrollbarLength = playfieldWidth * 0.5f;
    float posX = (playfieldWidth - scrollbarLength) / 2.0f;
    ImGui::SetCursorPos(ImVec2(posX, SCROLLBAR_PADDING));
    renderScrollbar();

    if (showMetadataPanel_) {
        renderMetadataPanel();
    }

    ImGui::End();
}

void PlayfieldOverlay::renderScrollbar() {
    if (!tables_ || tables_->empty() || tables_->size() <= 1) {
        return;
    }

    size_t numTables = tables_->size();
    float playfieldWidth = ImGui::GetWindowWidth();
    const float SCROLLBAR_LENGTH = playfieldWidth * 0.5f;
    float thumbWidth = THUMB_MIN_HEIGHT;

    float scrollProgress = (float)*currentIndex_ / (numTables - 1);
    float thumbXOffset = scrollProgress * (SCROLLBAR_LENGTH - thumbWidth);

    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ImVec2 cursorScreenPos = ImGui::GetCursorScreenPos();

    ImVec2 bgMin = cursorScreenPos;
    ImVec2 bgMax = ImVec2(bgMin.x + SCROLLBAR_LENGTH, bgMin.y + SCROLLBAR_WIDTH);
    draw_list->AddRectFilled(bgMin, bgMax, IM_COL32(50, 50, 50, 200), SCROLLBAR_WIDTH * 0.5f);

    ImVec2 thumbMin = ImVec2(bgMin.x + thumbXOffset, bgMin.y);
    ImVec2 thumbMax = ImVec2(bgMin.x + thumbXOffset + thumbWidth, bgMin.y + SCROLLBAR_WIDTH);
    draw_list->AddRectFilled(thumbMin, thumbMax, IM_COL32(150, 150, 150, 255), SCROLLBAR_WIDTH * 0.5f);

    ImGui::Dummy(ImVec2(SCROLLBAR_LENGTH, SCROLLBAR_WIDTH + SCROLLBAR_PADDING));
}

void PlayfieldOverlay::renderMetadataPanel() {
    if (!tables_ || tables_->empty() || *currentIndex_ >= tables_->size()) {
        LOG_DEBUG("PlayfieldOverlay: No table data to display in metadata panel");
        return;
    }

    const TableData& currentTable = tables_->at(*currentIndex_);

    float playfieldWidth = ImGui::GetWindowWidth();
    float playfieldHeight = ImGui::GetWindowHeight();

    float panelWidth = playfieldWidth * METADATA_PANEL_WIDTH_FACTOR;
    float panelHeight = playfieldHeight * METADATA_PANEL_HEIGHT_FACTOR;

    float posX = (playfieldWidth - panelWidth) / 2.0f;
    float posY = (playfieldHeight - panelHeight) / 2.0f;

    ImGui::SetNextWindowPos(ImVec2(posX, posY));
    ImGui::SetNextWindowSize(ImVec2(panelWidth, panelHeight));
    ImGui::SetNextWindowBgAlpha(METADATA_PANEL_ALPHA);

    if (ImGui::Begin("Table Metadata", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse)) {
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "TABLE INFO");
        std::filesystem::path filePath(currentTable.vpxFile);
        ImGui::Text("File: %s", filePath.filename().string().c_str());
        ImGui::Text("Table Name: %s", currentTable.tableName.c_str());
        if (!currentTable.vpsName.empty()) {
            ImGui::Text("VPS Name: %s", currentTable.vpsName.c_str());
        }
        ImGui::Text("ROM: %s", currentTable.gameName.c_str());
        ImGui::Text("Manufacturer: %s", currentTable.manufacturer.c_str());
        ImGui::Text("Year: %s", currentTable.year.c_str());
        if (!currentTable.type.empty()) {
            ImGui::Text("Type: %s", currentTable.type.c_str());
        }
        if (!currentTable.themes.empty()) {
            ImGui::Text("Themes: %s", currentTable.themes.c_str());
        }
        if (!currentTable.designers.empty()) {
            ImGui::Text("Designers: %s", currentTable.designers.c_str());
        }
        if (!currentTable.players.empty() && currentTable.players != "0") {
            ImGui::Text("Players: %s", currentTable.players.c_str());
        }
        if (!currentTable.ipdbUrl.empty()) {
            ImGui::Text("IPDB URL: %s", currentTable.ipdbUrl.c_str());
        }
        ImGui::Text("Release Date: %s", currentTable.releaseDate.c_str());
        ImGui::Text("Version: %s", currentTable.tableVersion.c_str());
        if (!currentTable.vpsVersion.empty()) {
            ImGui::Text("VPS Version: %s", currentTable.vpsVersion.c_str());
        }
        ImGui::Text("Revision: %s", currentTable.tableRevision.c_str());
        ImGui::Text("Save Date: %s", currentTable.tableSaveDate.c_str());
        ImGui::Text("Last Modified: %s", currentTable.lastModified.c_str());
        ImGui::Text("Authors: %s", currentTable.authorName.c_str());
        if (!currentTable.vpsAuthors.empty()) {
            ImGui::Text("VPS Authors: %s", currentTable.vpsAuthors.c_str());
        }
        if (!currentTable.features.empty()) {
            ImGui::Text("Features: %s", currentTable.features.c_str());
        }
        ImGui::Separator();
        ImGui::TextWrapped("Description: %s", currentTable.tableDescription.c_str());
        ImGui::End();
    }
}