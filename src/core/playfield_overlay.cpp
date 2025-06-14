/**
 * @file playfield_overlay.cpp
 * @brief Implements the PlayfieldOverlay class for rendering ImGui overlays in ASAPCabinetFE.
 *
 * This file provides the implementation for rendering ImGui-based UI elements, such as
scrollbars and metadata panels, on the playfield display. It integrates with table data,
configuration services, window management, and asset management.
 */

#include "core/playfield_overlay.h"
#include "config/settings.h"
#include "utils/logging.h"
#include <SDL.h>
#include <chrono>
#include <cmath>
#include <algorithm>

// Navigation Arrow Customization Settings
namespace NavigationArrowSettings {
    const Settings* settings = nullptr; // Pointer to settings
    bool SHOW_ARROWS() { return settings ? settings->showArrowHint : true; }
    float ARROW_HEIGHT() { return settings ? settings->arrowHintHeight : 100.0f; }
    float ARROW_WIDTH() { return settings ? settings->arrowHintWidth : 30.0f; }
    float LINE_THICKNESS() { return settings ? settings->arrowThickness : 4.0f; }
    float BASE_ALPHA() { return settings ? settings->arrowAlpha : 0.6f; }
    float GLOW_THICKNESS() { return settings ? settings->arrowGlow : 1.5f; }
    ImU32 GLOW_COLOR() {
        return settings ? IM_COL32(settings->arrowGlowColor.r, settings->arrowGlowColor.g,
                                  settings->arrowGlowColor.b, settings->arrowGlowColor.a)
                        : IM_COL32(200, 200, 200, 255);
    }
    ImU32 COLOR_TOP() {
        return settings ? IM_COL32(settings->arrowColorTop.r, settings->arrowColorTop.g,
                                  settings->arrowColorTop.b, settings->arrowColorTop.a)
                        : IM_COL32(100, 100, 100, 255);
    }
    ImU32 COLOR_BOTTOM() {
        return settings ? IM_COL32(settings->arrowColorBottom.r, settings->arrowColorBottom.g,
                                  settings->arrowColorBottom.b, settings->arrowColorBottom.a)
                        : IM_COL32(150, 150, 150, 255);
    }
}

// Scrollbar Customization Settings
namespace ScrollbarSettings {
    const Settings* settings = nullptr;
    bool SHOW_SCROLLBAR() { return settings ? settings->showScrollbar : true; }
    float WIDTH() { return settings ? settings->scrollbarWidth : 12.0f; }
    float PADDING() { return settings ? 15.0f : 15.0f; } // Static for now
    float THUMB_MIN_HEIGHT() { return settings ? settings->thumbWidth : 20.0f; }
    float LENGTH_FACTOR() { return settings ? settings->scrollbarLength : 0.5f; }
    ImU32 BACKGROUND_COLOR() {
        return settings ? IM_COL32(settings->scrollbarColor.r, settings->scrollbarColor.g,
                                  settings->scrollbarColor.b, settings->scrollbarColor.a)
                        : IM_COL32(50, 50, 50, 200);
    }
    ImU32 THUMB_COLOR() {
        return settings ? IM_COL32(settings->scrollbarThumbColor.r, settings->scrollbarThumbColor.g,
                                  settings->scrollbarThumbColor.b, settings->scrollbarThumbColor.a)
                        : IM_COL32(150, 150, 150, 255);
    }
    float CORNER_RADIUS() { return WIDTH() * 0.5f; }
}

// Metadata Panel Customization Settings
namespace MetadataPanelSettings {
    const Settings* settings = nullptr;
    float WIDTH_FACTOR() { return settings ? settings->metadataPanelWidth : 0.7f; }
    float HEIGHT_FACTOR() { return settings ? settings->metadataPanelHeight : 0.5f; }
    float ALPHA() { return settings ? settings->metadataPanelAlpha : 0.6f; }
}

PlayfieldOverlay::PlayfieldOverlay(const std::vector<TableData>* tables, size_t* currentIndex,
                                   IConfigService* configService, IWindowManager* windowManager,
                                   IAssetManager* assetManager, std::function<void()> refreshUICallback)
    : tables_(tables),
      currentIndex_(currentIndex),
      configService_(configService),
      windowManager_(windowManager),
      assetManager_(assetManager),
      showMetadataPanel_(configService->getSettings().showMetadata),
      resetMetadataFlags_(false),
      refreshUICallback_(refreshUICallback) {
    NavigationArrowSettings::settings = &configService_->getSettings();
    ScrollbarSettings::settings = &configService_->getSettings();
    MetadataPanelSettings::settings = &configService_->getSettings();
    LOG_INFO("Playfield Overlay Initialized.");
}

void PlayfieldOverlay::updateSettings(const Settings& settings) {
    showMetadataPanel_ = settings.showMetadata;
    LOG_DEBUG("PlayfieldOverlay: Updated showMetadataPanel to " << (showMetadataPanel_ ? "true" : "false"));
}

void PlayfieldOverlay::ResetMetadataFlags() {
    if (refreshUICallback_) {
        refreshUICallback_();
        LOG_DEBUG("PlayfieldOverlay: Refreshed ConfigUI via callback");
    }
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
                 ImGuiWindowFlags_NoInputs);

    if (ScrollbarSettings::SHOW_SCROLLBAR()) {
        float scrollbarLength = playfieldWidth * ScrollbarSettings::LENGTH_FACTOR();
        float posX = (playfieldWidth - scrollbarLength) / 2.0f;
        ImGui::SetCursorPos(ImVec2(posX, ScrollbarSettings::PADDING()));
        renderScrollbar();
    }

    if (showMetadataPanel_) {
        renderMetadataPanel();
    }

    // Navigation arrows with fade animation
    if (NavigationArrowSettings::SHOW_ARROWS()) {
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
        int glowR = (NavigationArrowSettings::GLOW_COLOR() >> 0) & 0xFF;
        int glowG = (NavigationArrowSettings::GLOW_COLOR() >> 8) & 0xFF;
        int glowB = (NavigationArrowSettings::GLOW_COLOR() >> 16) & 0xFF;
        // COLOR_TOP
        int topR = (NavigationArrowSettings::COLOR_TOP() >> 0) & 0xFF;
        int topG = (NavigationArrowSettings::COLOR_TOP() >> 8) & 0xFF;
        int topB = (NavigationArrowSettings::COLOR_TOP() >> 16) & 0xFF;
        // COLOR_BOTTOM
        int bottomR = (NavigationArrowSettings::COLOR_BOTTOM() >> 0) & 0xFF;
        int bottomG = (NavigationArrowSettings::COLOR_BOTTOM() >> 8) & 0xFF;
        int bottomB = (NavigationArrowSettings::COLOR_BOTTOM() >> 16) & 0xFF;

        // Apply alpha to colors
        ImU32 glowColor = IM_COL32(glowR, glowG, glowB, static_cast<int>(255 * 0.3f * leftAlpha));
        ImU32 colorTop = IM_COL32(topR, topG, topB, static_cast<int>(255 * NavigationArrowSettings::BASE_ALPHA() * leftAlpha));
        ImU32 colorBottom = IM_COL32(bottomR, bottomG, bottomB, static_cast<int>(255 * NavigationArrowSettings::BASE_ALPHA() * leftAlpha));

        float yPos = playfieldHeight / 2.0f - NavigationArrowSettings::ARROW_HEIGHT() / 2.0f;

        // Left arrow (chevron pointing left)
        float leftX = 20.0f; // Padding from left edge
        float chevronDepth = NavigationArrowSettings::ARROW_WIDTH() * 0.5f;
        ImVec2 leftTopStart(leftX + chevronDepth, yPos);
        ImVec2 leftTopEnd(leftX, yPos + NavigationArrowSettings::ARROW_HEIGHT() / 2.0f);
        ImVec2 leftBottomStart(leftX + chevronDepth, yPos + NavigationArrowSettings::ARROW_HEIGHT());
        ImVec2 leftBottomEnd(leftX, yPos + NavigationArrowSettings::ARROW_HEIGHT() / 2.0f);

        // Draw chevron lines with gradient
        drawList->AddLine(leftTopStart, leftTopEnd, colorTop, NavigationArrowSettings::LINE_THICKNESS());
        drawList->AddLine(leftBottomStart, leftBottomEnd, colorBottom, NavigationArrowSettings::LINE_THICKNESS());
        // Glow outline
        drawList->AddLine(leftTopStart, leftTopEnd, glowColor, NavigationArrowSettings::LINE_THICKNESS() + NavigationArrowSettings::GLOW_THICKNESS());
        drawList->AddLine(leftBottomStart, leftBottomEnd, glowColor, NavigationArrowSettings::LINE_THICKNESS() + NavigationArrowSettings::GLOW_THICKNESS());

        // Right arrow (chevron pointing right)
        float rightX = playfieldWidth - 50.0f; // Padding from right edge
        ImVec2 rightTopStart(rightX, yPos);
        ImVec2 rightTopEnd(rightX + chevronDepth, yPos + NavigationArrowSettings::ARROW_HEIGHT() / 2.0f);
        ImVec2 rightBottomStart(rightX, yPos + NavigationArrowSettings::ARROW_HEIGHT());
        ImVec2 rightBottomEnd(rightX + chevronDepth, yPos + NavigationArrowSettings::ARROW_HEIGHT() / 2.0f);

        // Draw chevron lines with gradient
        drawList->AddLine(rightTopStart, rightTopEnd, colorTop, NavigationArrowSettings::LINE_THICKNESS());
        drawList->AddLine(rightBottomStart, rightBottomEnd, colorBottom, NavigationArrowSettings::LINE_THICKNESS());
        // Glow outline
        drawList->AddLine(rightTopStart, rightTopEnd, glowColor, NavigationArrowSettings::LINE_THICKNESS() + NavigationArrowSettings::GLOW_THICKNESS());
        drawList->AddLine(rightBottomStart, rightBottomEnd, glowColor, NavigationArrowSettings::LINE_THICKNESS() + NavigationArrowSettings::GLOW_THICKNESS());
    }

    ImGui::End();
}

void PlayfieldOverlay::renderScrollbar() {
    if (!tables_ || tables_->empty() || tables_->size() <= 1) {
        return;
    }

    size_t numTables = tables_->size();
    float playfieldWidth = ImGui::GetWindowWidth();
    float thumbWidth = ScrollbarSettings::THUMB_MIN_HEIGHT();

    float scrollProgress = (float)*currentIndex_ / (numTables - 1);
    float thumbXOffset = scrollProgress * (playfieldWidth * ScrollbarSettings::LENGTH_FACTOR() - thumbWidth);

    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ImVec2 cursorScreenPos = ImGui::GetCursorScreenPos();

    ImVec2 bgMin = cursorScreenPos;
    ImVec2 bgMax = ImVec2(bgMin.x + playfieldWidth * ScrollbarSettings::LENGTH_FACTOR(), bgMin.y + ScrollbarSettings::WIDTH());
    draw_list->AddRectFilled(bgMin, bgMax, ScrollbarSettings::BACKGROUND_COLOR(), ScrollbarSettings::CORNER_RADIUS());

    ImVec2 thumbMin = ImVec2(bgMin.x + thumbXOffset, bgMin.y);
    ImVec2 thumbMax = ImVec2(bgMin.x + thumbXOffset + thumbWidth, bgMin.y + ScrollbarSettings::WIDTH());
    draw_list->AddRectFilled(thumbMin, thumbMax, ScrollbarSettings::THUMB_COLOR(), ScrollbarSettings::CORNER_RADIUS());

    ImGui::Dummy(ImVec2(playfieldWidth * ScrollbarSettings::LENGTH_FACTOR(), ScrollbarSettings::WIDTH() + ScrollbarSettings::PADDING()));
}

void PlayfieldOverlay::renderMetadataPanel() {
    if (!tables_ || tables_->empty() || *currentIndex_ >= tables_->size()) {
        return;
    }

    const TableData& currentTable = tables_->at(*currentIndex_);

    float playfieldWidth = ImGui::GetWindowWidth();
    float playfieldHeight = ImGui::GetWindowHeight();

    float panelWidth = playfieldWidth * MetadataPanelSettings::WIDTH_FACTOR();
    float panelHeight = playfieldHeight * MetadataPanelSettings::HEIGHT_FACTOR();

    float posX = (playfieldWidth - panelWidth) / 2.0f;
    float posY = (playfieldHeight - panelHeight) / 2.0f;

    ImGui::SetNextWindowPos(ImVec2(posX, posY));
    ImGui::SetNextWindowSize(ImVec2(panelWidth, panelHeight));
    ImGui::SetNextWindowBgAlpha(MetadataPanelSettings::ALPHA());

    if (ImGui::Begin("Table Metadata", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
                                                ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoInputs |
                                                ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize)) {
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "TABLE INFO");
        std::filesystem::path filePath(currentTable.vpxFile);
        
        // Essential fields, always display
        
        ImGui::Text("File: %s", filePath.filename().string().c_str());
        
        if (!currentTable.tableName.empty() && currentTable.tableName != filePath.stem().string()) {
            ImGui::Text("VPin Name: %s", currentTable.tableName.c_str()); // Fallback to local table name if title is same as filename
        }
        if (!currentTable.vpsName.empty()) {
            ImGui::Text("VPSdb Name: %s", currentTable.vpsName.c_str());
        }
        
        // ROM name special handling
        if (!currentTable.romName.empty()) {
             ImGui::Text("ROM: %s", currentTable.romName.c_str());
        }

        // Manufacturer and Year (combined if both are available, otherwise separately)
        bool hasManuf = !currentTable.manufacturer.empty();
        bool hasYear = !currentTable.year.empty();
        if (hasManuf && hasYear) {
            ImGui::Text("Manufacturer / Year: %s / %s", currentTable.manufacturer.c_str(), currentTable.year.c_str());
        } else if (hasManuf) {
            ImGui::Text("Manufacturer: %s", currentTable.manufacturer.c_str());
        } else if (hasYear) {
            ImGui::Text("Year: %s", currentTable.year.c_str());
        }

        if (currentTable.matchConfidence > 0) {
            // Map matchScore (0.0-1.0) to a 0-10 scale
            int fullStars = static_cast<int>(std::round(currentTable.matchConfidence * 10.0f));
            fullStars = std::clamp(fullStars, 0, 10); // Ensure it's between 0 and 10

            ImGui::Text("Match Confidence: ");
            ImGui::SameLine();
            ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 255, 0, 255)); // R, G, B, A for yellow
            // Render full stars
            for (int i = 0; i < fullStars; ++i) {
                ImGui::TextUnformatted("+"); // Full star
                ImGui::SameLine();
            }
            ImGui::PopStyleColor();
            // Render empty stars
            for (int i = fullStars; i < 10; ++i) {
                ImGui::TextUnformatted("-"); // Empty star
                ImGui::SameLine();
            }
            // Remove the last SameLine if you want text on the next line
            ImGui::NewLine(); // Move to the next line after stars
        }
        ImGui::Text("Source: %s", currentTable.jsonOwner.c_str());

        // --- VPSdb Specific Metadata ---
        bool vpsdb_section_started = false; // Flag to add a separator if any VPSdb field is printed

        if (!currentTable.vpsType.empty() || !currentTable.vpsThemes.empty() ||
            !currentTable.vpsDesigners.empty() || (!currentTable.vpsPlayers.empty() && currentTable.vpsPlayers != "0") ||
            !currentTable.vpsIpdbUrl.empty() || !currentTable.vpsAuthors.empty() ||
            !currentTable.vpsFeatures.empty() || !currentTable.vpsVersion.empty() ||
            !currentTable.vpsTableImgUrl.empty() || !currentTable.vpsTableUrl.empty() ||
            !currentTable.vpsManufacturer.empty() || !currentTable.vpsYear.empty() ||
            !currentTable.vpsId.empty()) {
            
            ImGui::Separator();
            ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "VPSDB DETAILS");
            vpsdb_section_started = true;
        }
        if (!currentTable.vpsId.empty()){
            ImGui::Text("ID: %s", currentTable.vpsId.c_str());
        }

        if (!currentTable.vpsManufacturer.empty()) {
            ImGui::Text("Manufacturer: %s", currentTable.vpsManufacturer.c_str());
        }
        if (!currentTable.vpsYear.empty()) {
            ImGui::Text("Year: %s", currentTable.vpsYear.c_str());
        }
        if (!currentTable.vpsType.empty()) {
            ImGui::Text("Type: %s", currentTable.vpsType.c_str());
        }
        if (!currentTable.vpsThemes.empty()) {
            ImGui::Text("Themes: %s", currentTable.vpsThemes.c_str());
        }
        if (!currentTable.vpsDesigners.empty()) {
            ImGui::Text("Designers: %s", currentTable.vpsDesigners.c_str());
        }
        if (!currentTable.vpsPlayers.empty() && currentTable.vpsPlayers != "0") {
            ImGui::Text("Players: %s", currentTable.vpsPlayers.c_str());
        }
        if (!currentTable.vpsIpdbUrl.empty()) {
            ImGui::Text("IPDB URL: %s", currentTable.vpsIpdbUrl.c_str());
        }
        if (!currentTable.vpsVersion.empty()) {
            ImGui::Text("Version: %s", currentTable.vpsVersion.c_str());
        }
        if (!currentTable.vpsAuthors.empty()) {
            ImGui::Text("Authors: %s", currentTable.vpsAuthors.c_str());
        }
        if (!currentTable.vpsFeatures.empty()) {
            ImGui::Text("Features: %s", currentTable.vpsFeatures.c_str());
        }
        if (!currentTable.vpsFormat.empty()) {
            ImGui::Text("Table Format: %s", currentTable.vpsFormat.c_str());
        }
        if (!currentTable.vpsTableImgUrl.empty()) {
            ImGui::Text("Table Image: %s", currentTable.vpsTableImgUrl.c_str());
        }
        if (!currentTable.vpsTableUrl.empty()) {
            ImGui::Text("Table Download: %s", currentTable.vpsTableUrl.c_str());
        }
        if (!currentTable.vpsB2SImgUrl.empty()) {
            ImGui::Text("Backglass Image: %s", currentTable.vpsB2SImgUrl.c_str());
        }
        if (!currentTable.vpsB2SUrl.empty()) {
            ImGui::Text("Backglass Download: %s", currentTable.vpsB2SUrl.c_str());
        }

        // --- File Metadata (from vpin/vpxtool) ---
        // Only add this section if any of these fields are available
        bool file_meta_section_started = false;
        if (!currentTable.tableAuthor.empty() || !currentTable.tableSaveDate.empty() ||
            !currentTable.tableLastModified.empty() || !currentTable.tableReleaseDate.empty() ||
            !currentTable.tableVersion.empty() || !currentTable.tableRevision.empty() ||
            !currentTable.tableBlurb.empty() || !currentTable.tableRules.empty() ||
            !currentTable.tableAuthorEmail.empty() || !currentTable.tableAuthorWebsite.empty() ||
            !currentTable.tableType.empty() || !currentTable.tableManufacturer.empty() || !currentTable.tableYear.empty()) {
            
            if (vpsdb_section_started) ImGui::Separator(); // Add separator only if previous section was displayed
            ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "FILE METADATA");
            file_meta_section_started = true;
        }

        if (!currentTable.tableName.empty() && currentTable.tableName == filePath.stem().string() && currentTable.title.empty()) {
             // If tableName is just the filename and no better title, show it here
             ImGui::Text("Table Name: %s", currentTable.tableName.c_str());
        }
        if (!currentTable.tableAuthor.empty()) {
            ImGui::Text("Author(s): %s", currentTable.tableAuthor.c_str());
        }
        if (!currentTable.tableSaveDate.empty()) {
            ImGui::Text("Save Date: %s", currentTable.tableSaveDate.c_str());
        }
        if (!currentTable.tableLastModified.empty()) {
            ImGui::Text("Last Modified: %s", currentTable.tableLastModified.c_str());
        }
        if (!currentTable.tableReleaseDate.empty()) {
            ImGui::Text("Release Date: %s", currentTable.tableReleaseDate.c_str());
        }
        // Only show Table Version if it's not already covered by VPSdb version (e.g., if it has "(Latest VPS: ...)")
        if (!currentTable.tableVersion.empty()) {
             if ((currentTable.vpsVersion.empty()) || !(currentTable.tableVersion.rfind(" (Latest VPS: ", 0) == 0)) {
                 ImGui::Text("Version: %s", currentTable.tableVersion.c_str());
             }
        }
        if (!currentTable.tableRevision.empty()) {
            ImGui::Text("Revision: %s", currentTable.tableRevision.c_str());
        }
        if (!currentTable.tableType.empty()) {
            ImGui::Text("Table Type: %s", currentTable.tableType.c_str());
        }
        if (!currentTable.tableManufacturer.empty()) {
            ImGui::Text("Company: %s", currentTable.tableManufacturer.c_str());
        }
        if (!currentTable.tableYear.empty()) {
            ImGui::Text("Company Year: %s", currentTable.tableYear.c_str());
        }
        if (!currentTable.tableBlurb.empty()) {
            ImGui::TextWrapped("Blurb: %s", currentTable.tableBlurb.c_str());
        }
        if (!currentTable.tableRules.empty()) {
            ImGui::TextWrapped("Rules: %s", currentTable.tableRules.c_str());
        }
        if (!currentTable.tableAuthorEmail.empty()) {
            ImGui::Text("Author Email: %s", currentTable.tableAuthorEmail.c_str());
        }
        if (!currentTable.tableAuthorWebsite.empty()) {
            ImGui::Text("Author Website: %s", currentTable.tableAuthorWebsite.c_str());
        }
        
        // --- Operational Tags / Status ---
        if (!currentTable.playCount.empty() || currentTable.hasPup || currentTable.altColor || currentTable.altSound ||
            currentTable.hasAltMusic || currentTable.hasUltraDMD) { // Add other operational tags here
            
            if (file_meta_section_started || vpsdb_section_started) ImGui::Separator();
            ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "STATUS & FEATURES");
        }

        if (!currentTable.playCount.empty()) {
            ImGui::Text("Play Count: %s", currentTable.playCount.c_str());
        }

        // Conditionally display boolean features on a single line
        std::string features_line;
        if (currentTable.hasPup) {
            features_line += "PUP ";
        }
        if (currentTable.altColor) {
            features_line += "AltColor ";
        }
        if (currentTable.altSound) {
            features_line += "AltSound ";
        }
        if (currentTable.hasAltMusic) {
            features_line += "AltMusic ";
        }
        if (currentTable.hasUltraDMD) {
            features_line += "UltraDMD ";
        }
        if (!features_line.empty()) {
            ImGui::Text("Found Assets: %s", features_line.c_str());
        }
        
        // --- Descriptions and Comments ---
        // Always add a separator before descriptions/comments if any previous section was displayed
        if (file_meta_section_started || vpsdb_section_started || !features_line.empty() || !currentTable.playCount.empty()) {
            ImGui::Separator();
        }
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "DESCRIPTION & COMMENTS");
        
        // Prefer vpsComment for tableDescription if it's more descriptive, otherwise show both if they differ
        if (!currentTable.tableDescription.empty()) {
            ImGui::TextWrapped("Description: %s", currentTable.tableDescription.c_str());
        }
        if (!currentTable.vpsComment.empty()) {
            ImGui::TextWrapped("Comments: %s", currentTable.vpsComment.c_str());
        }

        ImGui::End();
    }
}