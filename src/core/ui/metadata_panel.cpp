#include "metadata_panel.h"
#include <filesystem>
#include <cmath>
#include <algorithm>

void MetadataPanel::render(const TableData& currentTable, int playfieldWidth, int playfieldHeight, const Settings& settings) {
    ImGuiIO& io = ImGui::GetIO();
    bool isLandscape = io.DisplaySize.x > io.DisplaySize.y;

    // Panel sizing
    float panelWidth  = static_cast<float>(playfieldWidth)  * settings.metadataPanelWidth;
    float panelHeight = static_cast<float>(playfieldHeight) * settings.metadataPanelHeight;
    float posX = (static_cast<float>(playfieldWidth)  - panelWidth)  / 2.0f;
    float posY = (static_cast<float>(playfieldHeight) - panelHeight) / 2.0f;

    if (isLandscape) {
        posX = 0.0f;
        posY = 0.0f;
        panelWidth  = static_cast<float>(playfieldWidth);
        panelHeight = static_cast<float>(playfieldHeight);
    }

    ImGui::SetNextWindowPos(ImVec2(posX, posY));
    ImGui::SetNextWindowSize(ImVec2(panelWidth, panelHeight));
    ImGui::SetNextWindowBgAlpha(settings.metadataPanelAlpha);

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
                             ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoMove |
                             ImGuiWindowFlags_NoResize;

    if (ImGui::Begin("Table Metadata", nullptr, flags)) {
        if (isLandscape) {
            ImGui::Columns(2, "metadata_landscape");
            ImGui::BeginChild("metadata_info", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);
        }

        // ======== BASIC TABLE INFO ========
        std::filesystem::path filePath(currentTable.vpxFile);
        ImGui::TextColored(ImVec4(1, 1, 0, 1), "TABLE INFO");

        ImGui::Text("File: %s", filePath.filename().string().c_str());
        if (!currentTable.tableName.empty() && currentTable.tableName != filePath.stem().string())
            ImGui::Text("VPin Name: %s", currentTable.tableName.c_str());
        if (!currentTable.vpsName.empty())
            ImGui::Text("VPSdb Name: %s", currentTable.vpsName.c_str());
        if (!currentTable.title.empty() && currentTable.title != filePath.stem().string())
            ImGui::Text("Title: %s", currentTable.title.c_str());
        if (!currentTable.romName.empty())
            ImGui::Text("ROM: %s", currentTable.romName.c_str());

        bool hasManuf = !currentTable.manufacturer.empty();
        bool hasYear  = !currentTable.year.empty();
        if (hasManuf && hasYear)
            ImGui::Text("Manufacturer / Year: %s / %s", currentTable.manufacturer.c_str(), currentTable.year.c_str());
        else if (hasManuf)
            ImGui::Text("Manufacturer: %s", currentTable.manufacturer.c_str());
        else if (hasYear)
            ImGui::Text("Year: %s", currentTable.year.c_str());

        if (currentTable.matchConfidence > 0) {
            int fullStars = static_cast<int>(std::round(currentTable.matchConfidence * 10.0f));
            fullStars = std::clamp(fullStars, 0, 10);
            ImGui::Text("Match Confidence:");
            ImGui::SameLine();
            ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 255, 0, 255));
            for (int i = 0; i < fullStars; ++i) { ImGui::TextUnformatted("+"); ImGui::SameLine(); }
            ImGui::PopStyleColor();
            for (int i = fullStars; i < 10; ++i) { ImGui::TextUnformatted("-"); ImGui::SameLine(); }
            ImGui::NewLine();
        }
        ImGui::Text("Source: %s", currentTable.jsonOwner.c_str());

        // ======== VPSDB DETAILS (extended for editor mode) ========
        if (!currentTable.vpsId.empty() || !currentTable.vpsManufacturer.empty() ||
            !currentTable.vpsYear.empty() || !currentTable.vpsType.empty() ||
            !currentTable.vpsThemes.empty() || !currentTable.vpsDesigners.empty() ||
            !currentTable.vpsPlayers.empty() || !currentTable.vpsIpdbUrl.empty() ||
            !currentTable.vpsVersion.empty() || !currentTable.vpsAuthors.empty() ||
            !currentTable.vpsFeatures.empty() || !currentTable.vpsComment.empty() ||
            !currentTable.vpsFormat.empty()) {
            ImGui::Separator();
            ImGui::TextColored(ImVec4(1, 1, 0, 1), "VPSDB DETAILS");
        }

        if (!currentTable.vpsId.empty()) ImGui::Text("ID: %s", currentTable.vpsId.c_str());
        if (!currentTable.vpsManufacturer.empty()) ImGui::Text("Manufacturer: %s", currentTable.vpsManufacturer.c_str());
        if (!currentTable.vpsYear.empty()) ImGui::Text("Year: %s", currentTable.vpsYear.c_str());
        if (!currentTable.vpsType.empty()) ImGui::Text("Type: %s", currentTable.vpsType.c_str());
        if (!currentTable.vpsThemes.empty()) ImGui::Text("Themes: %s", currentTable.vpsThemes.c_str());
        if (!currentTable.vpsDesigners.empty()) ImGui::Text("Designers: %s", currentTable.vpsDesigners.c_str());
        if (!currentTable.vpsPlayers.empty()) ImGui::Text("Players: %s", currentTable.vpsPlayers.c_str());
        if (!currentTable.vpsIpdbUrl.empty()) ImGui::Text("IPDB URL: %s", currentTable.vpsIpdbUrl.c_str());
        if (!currentTable.vpsVersion.empty()) ImGui::Text("Version: %s", currentTable.vpsVersion.c_str());
        if (!currentTable.vpsAuthors.empty()) ImGui::Text("Authors: %s", currentTable.vpsAuthors.c_str());
        if (!currentTable.vpsFeatures.empty()) ImGui::Text("Features: %s", currentTable.vpsFeatures.c_str());
        if (!currentTable.vpsFormat.empty()) ImGui::Text("Format: %s", currentTable.vpsFormat.c_str());
        if (!currentTable.vpsComment.empty() && isLandscape)
            ImGui::TextWrapped("Comment: %s", currentTable.vpsComment.c_str());

        // ======== FILE METADATA (editor focus only) ========
        if (isLandscape) {
            ImGui::Separator();
            ImGui::TextColored(ImVec4(1, 1, 0, 1), "FILE METADATA");

            if (!currentTable.tableAuthor.empty()) ImGui::Text("Author: %s", currentTable.tableAuthor.c_str());
            if (!currentTable.tableAuthorEmail.empty()) ImGui::Text("Email: %s", currentTable.tableAuthorEmail.c_str());
            if (!currentTable.tableAuthorWebsite.empty()) ImGui::Text("Website: %s", currentTable.tableAuthorWebsite.c_str());
            if (!currentTable.tableVersion.empty()) ImGui::Text("Version: %s", currentTable.tableVersion.c_str());
            if (!currentTable.tableRevision.empty()) ImGui::Text("Revision: %s", currentTable.tableRevision.c_str());
            if (!currentTable.tableReleaseDate.empty()) ImGui::Text("Release: %s", currentTable.tableReleaseDate.c_str());
            if (!currentTable.tableSaveDate.empty()) ImGui::Text("Saved: %s", currentTable.tableSaveDate.c_str());
            if (!currentTable.tableLastModified.empty()) ImGui::Text("Last Modified: %s", currentTable.tableLastModified.c_str());
            if (!currentTable.tableManufacturer.empty()) ImGui::Text("Company: %s", currentTable.tableManufacturer.c_str());
            if (!currentTable.tableYear.empty()) ImGui::Text("Year: %s", currentTable.tableYear.c_str());
            if (!currentTable.tableType.empty()) ImGui::Text("Type: %s", currentTable.tableType.c_str());
            if (!currentTable.tableBlurb.empty()) ImGui::TextWrapped("Blurb: %s", currentTable.tableBlurb.c_str());
            if (!currentTable.tableRules.empty()) ImGui::TextWrapped("Rules: %s", currentTable.tableRules.c_str());
        }

        // ======== STATUS ========
        ImGui::Separator();
        ImGui::TextColored(ImVec4(1, 1, 0, 1), "STATUS & FEATURES");

        std::string features_line;
        if (currentTable.hasPup) features_line += "PUP ";
        if (currentTable.hasAltColor) features_line += "AltColor ";
        if (currentTable.hasAltSound) features_line += "AltSound ";
        if (currentTable.hasAltMusic) features_line += "AltMusic ";
        if (currentTable.hasUltraDMD) features_line += "UltraDMD ";
        if (!features_line.empty())
            ImGui::Text("Found Assets: %s", features_line.c_str());

        if (isLandscape) {
            ImGui::Text("Play Count: %d", currentTable.playCount);
            ImGui::Text("Total Playtime: %.1f min", currentTable.playTimeTotal / 60.0f);
            if (currentTable.isBroken) ImGui::TextColored(ImVec4(1, 0.3f, 0.3f, 1), "Status: BROKEN");
        }

        // ======== DESCRIPTION ========
        ImGui::Separator();
        ImGui::TextColored(ImVec4(1, 1, 0, 1), "DESCRIPTION & COMMENTS");
        if (!currentTable.tableDescription.empty())
            ImGui::TextWrapped("Description: %s", currentTable.tableDescription.c_str());
        if (isLandscape && !currentTable.vpsComment.empty())
            ImGui::TextWrapped("VPS Comments: %s", currentTable.vpsComment.c_str());

//======================================== LANDSCAPE MEDIA PREVIEW ======================================================
        if (isLandscape) {
            ImGui::EndChild();
            ImGui::NextColumn();
            ImGui::BeginChild("metadata_media", ImVec2(0, 0), true, ImGuiWindowFlags_HorizontalScrollbar);
            ImGui::TextColored(ImVec4(1, 1, 0, 1), "MEDIA PREVIEW");

            // Helper lambda to show image/video pair
            auto drawMedia = [&](const char* label, const std::string& imagePath, const std::string& videoPath,
                                bool hasImage, bool hasVideo) {
                if (!hasImage && !hasVideo)
                    return;

                ImGui::Separator();
                ImGui::TextColored(ImVec4(0.8f, 0.8f, 1.0f, 1.0f), "%s", label);

                if (hasImage) {
                    ImGui::Text("Image:");
                    // TODO: replace with actual texture ID when integrated
                    ImGui::Button(("[IMG] " + std::filesystem::path(imagePath).filename().string()).c_str(), ImVec2(-FLT_MIN, 100));
                }

                if (hasVideo) {
                    ImGui::Text("Video:");
                    // TODO: show static frame or preview thumbnail (temporary button now)
                    if (ImGui::Button(("▶ " + std::filesystem::path(videoPath).filename().string()).c_str(), ImVec2(-FLT_MIN, 30))) {
                        // TODO: hook into video preview player
                    }
                }
            };

            // Draw each media section (conditioned by has* flags)
            drawMedia("Playfield", currentTable.playfieldImage, currentTable.playfieldVideo,
                    currentTable.hasPlayfieldImage, currentTable.hasPlayfieldVideo);

            drawMedia("Backglass", currentTable.backglassImage, currentTable.backglassVideo,
                    currentTable.hasBackglassImage, currentTable.hasBackglassVideo);

            drawMedia("DMD", currentTable.dmdImage, currentTable.dmdVideo,
                    currentTable.hasDmdImage, currentTable.hasDmdVideo);

            drawMedia("Topper", currentTable.topperImage, currentTable.topperVideo,
                    currentTable.hasTopperImage, currentTable.hasTopperVideo);

            // Wheel image (custom only)
            if (currentTable.hasWheelImage) {
                ImGui::Separator();
                ImGui::TextColored(ImVec4(0.8f, 1.0f, 0.8f, 1.0f), "WHEEL IMAGE");
                ImGui::Button(("[IMG] " + std::filesystem::path(currentTable.wheelImage).filename().string()).c_str(), ImVec2(-FLT_MIN, 80));
            }

            // Custom music and launch sound
            if (currentTable.hasTableMusic || currentTable.hasLaunchAudio) {
                ImGui::Separator();
                ImGui::TextColored(ImVec4(1.0f, 0.9f, 0.6f, 1.0f), "AUDIO PREVIEW");

                if (currentTable.hasTableMusic) {
                    if (ImGui::Button("▶ Play Table Music", ImVec2(-FLT_MIN, 30))) {
                        // TODO: hook into audio preview function
                    }
                }

                if (currentTable.hasLaunchAudio) {
                    if (ImGui::Button("▶ Play Launch Sound", ImVec2(-FLT_MIN, 30))) {
                        // TODO: hook into audio preview function
                    }
                }
            }

            ImGui::EndChild();
            ImGui::Columns(1);
        }


        ImGui::End();
    }
}
