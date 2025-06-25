#include "metadata_panel.h"
#include <filesystem>
#include <cmath>
#include <algorithm>

void MetadataPanel::render(const TableData& currentTable, int playfieldWidth, int playfieldHeight, const Settings& settings) {
    float panelWidth = playfieldWidth * settings.metadataPanelWidth;
    float panelHeight = playfieldHeight * settings.metadataPanelHeight;

    float posX = (playfieldWidth - panelWidth) / 2.0f;
    float posY = (playfieldHeight - panelHeight) / 2.0f;

    ImGui::SetNextWindowPos(ImVec2(posX, posY));
    ImGui::SetNextWindowSize(ImVec2(panelWidth, panelHeight));
    ImGui::SetNextWindowBgAlpha(settings.metadataPanelAlpha);

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
        // these make no sense here, but maybe we can do something fun after we set
        // config toggles for the fields for user customization.
        // if (!currentTable.vpsTableImgUrl.empty()) {
        //     ImGui::Text("Table Image: %s", currentTable.vpsTableImgUrl.c_str());
        // }
        // if (!currentTable.vpsTableUrl.empty()) {
        //     ImGui::Text("Table Download: %s", currentTable.vpsTableUrl.c_str());
        // }
        // if (!currentTable.vpsB2SImgUrl.empty()) {
        //     ImGui::Text("Backglass Image: %s", currentTable.vpsB2SImgUrl.c_str());
        // }
        // if (!currentTable.vpsB2SUrl.empty()) {
        //     ImGui::Text("Backglass Download: %s", currentTable.vpsB2SUrl.c_str());
        // }

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
        if (currentTable.playCount > 0 || currentTable.hasPup || currentTable.hasAltColor || currentTable.hasAltSound ||
            currentTable.hasAltMusic || currentTable.hasUltraDMD) { // Add other operational tags here
            
            if (file_meta_section_started || vpsdb_section_started) ImGui::Separator();
            ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "STATUS & FEATURES");
        }

        if (currentTable.playCount > 0) {
            ImGui::Text("Play Count: %i", currentTable.playCount);
        }

        if (currentTable.playTimeLast > 0) {
            int totalSeconds = static_cast<int>(currentTable.playTimeLast);
            int hours = totalSeconds / 3600;
            int minutes = (totalSeconds % 3600) / 60;
            int seconds = totalSeconds % 60;
            char timeStr[16];
            sprintf(timeStr, "%02d:%02d:%02d", hours, minutes, seconds);
            ImGui::Text("Last Session Playtime: %s", timeStr);
        }

        if (currentTable.playTimeTotal > 0) {
            int totalSeconds = static_cast<int>(currentTable.playTimeTotal);
            int hours = totalSeconds / 3600;
            int minutes = (totalSeconds % 3600) / 60;
            int seconds = totalSeconds % 60;
            char timeStr[16];
            sprintf(timeStr, "%02d:%02d:%02d", hours, minutes, seconds);
            ImGui::Text("Total Time Played: %s", timeStr);
        }

        if (currentTable.isBroken) {
            ImGui::Text("Last launch: ");
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "FAILED!");
        }

        // Conditionally display boolean features on a single line
        std::string features_line;
        if (currentTable.hasPup) {
            features_line += "PUP ";
        }
        if (currentTable.hasAltColor) {
            features_line += "AltColor ";
        }
        if (currentTable.hasAltSound) {
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
        if (file_meta_section_started || vpsdb_section_started || !features_line.empty()) {
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