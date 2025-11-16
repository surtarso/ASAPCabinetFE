#include "metadata_panel.h"
#include <filesystem>
#include <cmath>
#include <algorithm>

void MetadataPanel::render(const TableData& currentTable,
                           int playfieldWidth,
                           int playfieldHeight,
                           const Settings& settings)
{
    // FE version → explicitly forward to the main implementation with uiRenderer = nullptr
    render(currentTable, playfieldWidth, playfieldHeight, settings, nullptr);
}

void MetadataPanel::render(const TableData& currentTable,
                           int playfieldWidth,
                           int playfieldHeight,
                           const Settings& settings,
                           SDL_Renderer* uiRenderer)
{
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

    bool isOpen = ImGui::Begin("Table Metadata", nullptr, flags);

    if (!isOpen) {
        if (wasOpen_)
            MediaPreview::instance().clearMemoryCache();
        wasOpen_ = false;
        ImGui::End();
        return;
    }

    wasOpen_ = true;

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

    // ======== VPSDB DETAILS ========
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

    // ======== LANDSCAPE MODE MEDIA PREVIEW ========
    if (isLandscape) {
        ImGui::Columns(2, "metadata_landscape");
        ImGui::BeginChild("metadata_info", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);
        // ... draw table info (already drawn above) ...
        ImGui::EndChild();

        ImGui::NextColumn();
        ImGui::BeginChild("metadata_media", ImVec2(0, 0), true, ImGuiWindowFlags_HorizontalScrollbar);
        ImGui::TextColored(ImVec4(1, 1, 0, 1), "MEDIA PREVIEW");

        auto drawMedia = [&](const char* label, const std::string& imagePath, const std::string& videoPath,
                             bool hasImage, bool hasVideo)
        {
            if (!hasImage && !hasVideo) return;

            ImGui::Separator();
            ImGui::TextColored(ImVec4(0.8f, 0.8f, 1.0f, 1.0f), "%s", label);

            if (uiRenderer == nullptr) {
                if (hasImage) ImGui::Text("[image]");
                if (hasVideo) ImGui::Text("[video]");
                return;
            }

            const int thumbHeight = 160;

            if (hasImage) {
                ImGui::Text("Image:");
                SDL_Texture* tex = MediaPreview::instance().getThumbnail(uiRenderer, imagePath, thumbHeight);
                if (tex) {
                    int w = 0, h = 0;
                    SDL_QueryTexture(tex, nullptr, nullptr, &w, &h);
                    float scale = float(thumbHeight) / float(h);
                    ImVec2 size(float(w) * scale, float(h) * scale);
                    ImGui::Image((void*)tex, size);
                } else {
                    ImGui::TextColored(ImVec4(1, 0.3f, 0.3f, 1), "Failed to load thumbnail");
                }
            }

            if (hasVideo) {
                ImGui::Text("Video snapshot:");
                SDL_Texture* tex = MediaPreview::instance().getThumbnail(uiRenderer, videoPath, thumbHeight);
                if (tex) {
                    int w = 0, h = 0;
                    SDL_QueryTexture(tex, nullptr, nullptr, &w, &h);
                    float scale = float(thumbHeight) / float(h);
                    ImVec2 size(float(w) * scale, float(h) * scale);
                    ImGui::Image((void*)tex, size);
                } else {
                    ImGui::TextColored(ImVec4(1, 0.3f, 0.3f, 1), "Failed to preview video");
                }
            }
        };

        drawMedia("Playfield", currentTable.playfieldImage, currentTable.playfieldVideo,
                  currentTable.hasPlayfieldImage, currentTable.hasPlayfieldVideo);
        drawMedia("Backglass", currentTable.backglassImage, currentTable.backglassVideo,
                  currentTable.hasBackglassImage, currentTable.hasBackglassVideo);
        drawMedia("DMD", currentTable.dmdImage, currentTable.dmdVideo,
                  currentTable.hasDmdImage, currentTable.hasDmdVideo);
        drawMedia("Topper", currentTable.topperImage, currentTable.topperVideo,
                  currentTable.hasTopperImage, currentTable.hasTopperVideo);

        if (currentTable.hasWheelImage && uiRenderer) {
            const int thumbHeight = 160;
            SDL_Texture* tex = MediaPreview::instance().getThumbnail(uiRenderer, currentTable.wheelImage, thumbHeight);
            if (tex) {
                int w = 0, h = 0;
                SDL_QueryTexture(tex, nullptr, nullptr, &w, &h);
                float scale = float(thumbHeight) / float(h);
                ImVec2 size(float(w) * scale, float(h) * scale);
                ImGui::Image((void*)tex, size);
            } else {
                ImGui::TextColored(ImVec4(1, 0.3f, 0.3f, 1), "Failed to load wheel");
            }
        }

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
