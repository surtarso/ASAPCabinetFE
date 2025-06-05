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
#include <chrono>
#include <cmath>
#include <algorithm>

// Navigation Arrow Customization Settings
// These constants control the appearance and behavior of the navigation arrows.
// They can be integrated into configUI for user customization.
namespace NavigationArrowSettings {
    const bool SHOW_ARROWS = true;              ///< Toggle to show/hide navigation arrows
    const float ARROW_HEIGHT = 100.0f;          ///< Height of the arrows in pixels
    const float ARROW_WIDTH = 30.0f;            ///< Width of the arrows in pixels
    const float LINE_THICKNESS = 4.0f;          ///< Thickness of the chevron lines
    const float BASE_ALPHA = 0.6f;              ///< Base transparency (0.0f to 1.0f)
    const float GLOW_THICKNESS = 1.5f;          ///< Glow outline thickness
    const ImU32 GLOW_COLOR = IM_COL32(200, 200, 200, 255);  ///< Light gray glow (RGBA)
    const ImU32 COLOR_TOP = IM_COL32(100, 100, 100, 255);   ///< Dark gray for top of gradient (RGBA)
    const ImU32 COLOR_BOTTOM = IM_COL32(150, 150, 150, 255);///< Light gray for bottom of gradient (RGBA)
}

// Scrollbar Customization Settings
// These constants control the appearance and behavior of the scrollbar.
// They can be integrated into configUI for user customization.
namespace ScrollbarSettings {
    const bool SHOW_SCROLLBAR = true;           ///< Toggle to show/hide scrollbar
    const float WIDTH = 12.0f;                  ///< Width of the scrollbar bar and thumb
    const float PADDING = 15.0f;                ///< Padding from the edge of the screen
    const float THUMB_MIN_HEIGHT = 20.0f;       ///< Minimum height for the scrollbar thumb
    const float LENGTH_FACTOR = 0.5f;           ///< Scrollbar length as a fraction of playfield width
    const ImU32 BACKGROUND_COLOR = IM_COL32(50, 50, 50, 200);  ///< Background color (RGBA)
    const ImU32 THUMB_COLOR = IM_COL32(150, 150, 150, 255);    ///< Thumb color (RGBA)
    const float CORNER_RADIUS = ScrollbarSettings::WIDTH * 0.5f;  ///< Corner radius for rounded edges
}

// Metadata Panel Customization Settings
// These constants control the appearance and behavior of the metadata panel.
// They can be integrated into configUI for user customization.
namespace MetadataPanelSettings {
    const float WIDTH_FACTOR = 0.7f;            ///< Metadata panel takes 70% of playfield width
    const float HEIGHT_FACTOR = 0.5f;           ///< Metadata panel takes 50% of playfield height
    const float ALPHA = 0.6f;                   ///< Transparency for the metadata panel background
}

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
    LOG_INFO("Playfield Overlay Initialized.");
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

    if (ScrollbarSettings::SHOW_SCROLLBAR) {
        float scrollbarLength = playfieldWidth * ScrollbarSettings::LENGTH_FACTOR;
        float posX = (playfieldWidth - scrollbarLength) / 2.0f;
        ImGui::SetCursorPos(ImVec2(posX, ScrollbarSettings::PADDING));
        renderScrollbar();
    }

    if (showMetadataPanel_) {
        renderMetadataPanel();
    }

    // Navigation arrows with fade animation
    if (NavigationArrowSettings::SHOW_ARROWS) {
        static auto lastTime = std::chrono::steady_clock::now();
        static float leftAlpha = 0.0f;
        static float rightAlpha = 0.0f;
        static bool fadingIn = true;

        auto currentTime = std::chrono::steady_clock::now();
        float deltaTime = std::chrono::duration<float>(currentTime - lastTime).count();
        lastTime = currentTime;

        // Fade animation: oscillate between 0.2 and 1.0
        const float animationSpeed = 1.0f;
        if (fadingIn) {
            leftAlpha += animationSpeed * deltaTime;
            rightAlpha += animationSpeed * deltaTime;
            if (leftAlpha >= 1.0f) fadingIn = false;
        } else {
            leftAlpha -= animationSpeed * deltaTime;
            rightAlpha -= animationSpeed * deltaTime;
            if (leftAlpha <= 0.2f) fadingIn = true;
        }
        leftAlpha = std::clamp(leftAlpha, 0.2f, 1.0f);
        rightAlpha = std::clamp(rightAlpha, 0.2f, 1.0f);

        ImDrawList* drawList = ImGui::GetWindowDrawList();

        // Extract ABGR channels correctly (ImU32 is ABGR, not RGBA)
        // GLOW_COLOR
        int glowR = (NavigationArrowSettings::GLOW_COLOR >> 0) & 0xFF;
        int glowG = (NavigationArrowSettings::GLOW_COLOR >> 8) & 0xFF;
        int glowB = (NavigationArrowSettings::GLOW_COLOR >> 16) & 0xFF;
        // COLOR_TOP
        int topR = (NavigationArrowSettings::COLOR_TOP >> 0) & 0xFF;
        int topG = (NavigationArrowSettings::COLOR_TOP >> 8) & 0xFF;
        int topB = (NavigationArrowSettings::COLOR_TOP >> 16) & 0xFF;
        // COLOR_BOTTOM
        int bottomR = (NavigationArrowSettings::COLOR_BOTTOM >> 0) & 0xFF;
        int bottomG = (NavigationArrowSettings::COLOR_BOTTOM >> 8) & 0xFF;
        int bottomB = (NavigationArrowSettings::COLOR_BOTTOM >> 16) & 0xFF;

        // Apply alpha to colors
        ImU32 glowColor = IM_COL32(glowR, glowG, glowB, static_cast<int>(255 * 0.3f * leftAlpha));
        ImU32 colorTop = IM_COL32(topR, topG, topB, static_cast<int>(255 * NavigationArrowSettings::BASE_ALPHA * leftAlpha));
        ImU32 colorBottom = IM_COL32(bottomR, bottomG, bottomB, static_cast<int>(255 * NavigationArrowSettings::BASE_ALPHA * leftAlpha));

        float yPos = playfieldHeight / 2.0f - NavigationArrowSettings::ARROW_HEIGHT / 2.0f;

        // Left arrow (chevron pointing left)
        float leftX = 20.0f;  // Padding from left edge
        float chevronDepth = NavigationArrowSettings::ARROW_WIDTH * 0.5f;  // Depth of the chevron point
        ImVec2 leftTopStart(leftX + chevronDepth, yPos);
        ImVec2 leftTopEnd(leftX, yPos + NavigationArrowSettings::ARROW_HEIGHT / 2.0f);
        ImVec2 leftBottomStart(leftX + chevronDepth, yPos + NavigationArrowSettings::ARROW_HEIGHT);
        ImVec2 leftBottomEnd(leftX, yPos + NavigationArrowSettings::ARROW_HEIGHT / 2.0f);

        // Draw chevron lines with gradient
        drawList->AddLine(leftTopStart, leftTopEnd, colorTop, NavigationArrowSettings::LINE_THICKNESS);
        drawList->AddLine(leftBottomStart, leftBottomEnd, colorBottom, NavigationArrowSettings::LINE_THICKNESS);
        // Glow outline
        drawList->AddLine(leftTopStart, leftTopEnd, glowColor, NavigationArrowSettings::LINE_THICKNESS + NavigationArrowSettings::GLOW_THICKNESS);
        drawList->AddLine(leftBottomStart, leftBottomEnd, glowColor, NavigationArrowSettings::LINE_THICKNESS + NavigationArrowSettings::GLOW_THICKNESS);

        // Right arrow (chevron pointing right)
        float rightX = playfieldWidth - 50.0f;  // Padding from right edge
        ImVec2 rightTopStart(rightX, yPos);
        ImVec2 rightTopEnd(rightX + chevronDepth, yPos + NavigationArrowSettings::ARROW_HEIGHT / 2.0f);
        ImVec2 rightBottomStart(rightX, yPos + NavigationArrowSettings::ARROW_HEIGHT);
        ImVec2 rightBottomEnd(rightX + chevronDepth, yPos + NavigationArrowSettings::ARROW_HEIGHT / 2.0f);

        // Draw chevron lines with gradient
        drawList->AddLine(rightTopStart, rightTopEnd, colorTop, NavigationArrowSettings::LINE_THICKNESS);
        drawList->AddLine(rightBottomStart, rightBottomEnd, colorBottom, NavigationArrowSettings::LINE_THICKNESS);
        // Glow outline
        drawList->AddLine(rightTopStart, rightTopEnd, glowColor, NavigationArrowSettings::LINE_THICKNESS + NavigationArrowSettings::GLOW_THICKNESS);
        drawList->AddLine(rightBottomStart, rightBottomEnd, glowColor, NavigationArrowSettings::LINE_THICKNESS + NavigationArrowSettings::GLOW_THICKNESS);
    }

    ImGui::End();
}

void PlayfieldOverlay::renderScrollbar() {
    if (!tables_ || tables_->empty() || tables_->size() <= 1) {
        return;
    }

    size_t numTables = tables_->size();
    float playfieldWidth = ImGui::GetWindowWidth();
    float thumbWidth = ScrollbarSettings::THUMB_MIN_HEIGHT;

    float scrollProgress = (float)*currentIndex_ / (numTables - 1);
    float thumbXOffset = scrollProgress * (playfieldWidth * ScrollbarSettings::LENGTH_FACTOR - thumbWidth);

    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ImVec2 cursorScreenPos = ImGui::GetCursorScreenPos();

    ImVec2 bgMin = cursorScreenPos;
    ImVec2 bgMax = ImVec2(bgMin.x + playfieldWidth * ScrollbarSettings::LENGTH_FACTOR, bgMin.y + ScrollbarSettings::WIDTH);
    draw_list->AddRectFilled(bgMin, bgMax, ScrollbarSettings::BACKGROUND_COLOR, ScrollbarSettings::CORNER_RADIUS);

    ImVec2 thumbMin = ImVec2(bgMin.x + thumbXOffset, bgMin.y);
    ImVec2 thumbMax = ImVec2(bgMin.x + thumbXOffset + thumbWidth, bgMin.y + ScrollbarSettings::WIDTH);
    draw_list->AddRectFilled(thumbMin, thumbMax, ScrollbarSettings::THUMB_COLOR, ScrollbarSettings::CORNER_RADIUS);

    ImGui::Dummy(ImVec2(playfieldWidth * ScrollbarSettings::LENGTH_FACTOR, ScrollbarSettings::WIDTH + ScrollbarSettings::PADDING));
}

void PlayfieldOverlay::renderMetadataPanel() {
    if (!tables_ || tables_->empty() || *currentIndex_ >= tables_->size()) {
        LOG_DEBUG("PlayfieldOverlay: No table data to display in metadata panel");
        return;
    }

    const TableData& currentTable = tables_->at(*currentIndex_);

    float playfieldWidth = ImGui::GetWindowWidth();
    float playfieldHeight = ImGui::GetWindowHeight();

    float panelWidth = playfieldWidth * MetadataPanelSettings::WIDTH_FACTOR;
    float panelHeight = playfieldHeight * MetadataPanelSettings::HEIGHT_FACTOR;

    float posX = (playfieldWidth - panelWidth) / 2.0f;
    float posY = (playfieldHeight - panelHeight) / 2.0f;

    ImGui::SetNextWindowPos(ImVec2(posX, posY));
    ImGui::SetNextWindowSize(ImVec2(panelWidth, panelHeight));
    ImGui::SetNextWindowBgAlpha(MetadataPanelSettings::ALPHA);

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