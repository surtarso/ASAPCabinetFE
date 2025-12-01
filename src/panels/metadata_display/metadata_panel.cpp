#include "panels/metadata_display/metadata_panel.h"
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

    // --- 1. METADATA TEXT CONTENT DRAWING LAMBDA ---
    // Contains all the basic info and VPS details.
    auto DrawInfoContent = [&]() {
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

        // ADD FLYER IMAGES HERE (shown only in portrait mode)

        // We now call this lambda in both modes, so we keep the condition to prevent showing it in FE.
        if (!currentTable.vpsComment.empty() && isLandscape)
            ImGui::TextWrapped("Comment: %s", currentTable.vpsComment.c_str());
    };

    // --- 2. MEDIA PREVIEW AND AUDIO DRAWING LAMBDA (Used only in Landscape/Editor) ---
    auto DrawMediaContent = [&]() {

        ImGui::TextColored(ImVec4(1, 1, 0, 1), "MEDIA PREVIEW");

        // Helper to draw media (image and/or video) side-by-side or stacked
        auto drawMediaPair = [&](const char* label,
                                 const std::string& imagePath, const std::string& videoPath,
                                 bool hasImage, bool hasVideo,
                                 bool sideBySide = true)
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

            if (sideBySide) {
                ImGui::BeginGroup(); // Group 1 (Image)
            }

            // --------------------------------- IMAGE PREVIEW ----------------------------------
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

            // --------------------------------- VIDEO PREVIEW -----------------------------------
            if (hasVideo) {
                if (sideBySide && hasImage) {
                    ImGui::SameLine();
                }

                if (sideBySide) {
                    ImGui::BeginGroup(); // Group 2 (Video)
                }

                ImGui::Text("Video:");
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

                if (sideBySide) {
                    ImGui::EndGroup(); // End Group 2
                }
            }

            if (sideBySide) {
                ImGui::EndGroup(); // End Group 1 (or the outer group)
            }
        };

        // ---------------------- Media Layout Configuration ----------------------
        // Playfield, Backglass, Topper: Side-by-Side
        drawMediaPair("Playfield", currentTable.playfieldImage, currentTable.playfieldVideo,
                        currentTable.hasPlayfieldImage, currentTable.hasPlayfieldVideo, true);

        drawMediaPair("Backglass", currentTable.backglassImage, currentTable.backglassVideo,
                        currentTable.hasBackglassImage, currentTable.hasBackglassVideo, true);

        drawMediaPair("Topper", currentTable.topperImage, currentTable.topperVideo,
                        currentTable.hasTopperImage, currentTable.hasTopperVideo, true);

        // DMD: Stacked (since art is "ultrawide")
        drawMediaPair("DMD", currentTable.dmdImage, currentTable.dmdVideo,
                        currentTable.hasDmdImage, currentTable.hasDmdVideo, false);

        drawMediaPair("Flyer", currentTable.flyerFront, currentTable.flyerBack,
                        currentTable.hasFlyerFront, currentTable.hasFlyerBack, true);

        // Wheel (stacked)
        if (currentTable.hasWheelImage && uiRenderer) {
            ImGui::Separator();
            ImGui::TextColored(ImVec4(0.8f, 0.8f, 1.0f, 1.0f), "Wheel");
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

        // --------------------------- AUDIO PREVIEW SECTION ------------------------------------
        if (currentTable.hasTableMusic || currentTable.hasLaunchAudio) {
            ImGui::Separator();
            ImGui::TextColored(ImVec4(1.0f, 0.9f, 0.6f, 1.0f), "AUDIO PREVIEW");

            // Table Music
            if (currentTable.hasTableMusic) {
                ImGui::Text("Table Music:");
                ImGui::SameLine();

                if (ImGui::Button("Play##Music")) { // Play
                    if (!currentTable.music.empty() && std::filesystem::exists(currentTable.music))
                        soundManager_->playTableMusic(currentTable.music);
                }
                ImGui::SameLine();
                if (ImGui::Button("Stop##Music")) { // Stop
                    soundManager_->stopMusic();
                }
            }

            // Launch Audio
            if (currentTable.hasLaunchAudio) {
                ImGui::Text("Launch Audio:");
                ImGui::SameLine();

                if (ImGui::Button("Play##Launch")) { // Play
                    if (!currentTable.launchAudio.empty() && std::filesystem::exists(currentTable.launchAudio))
                        soundManager_->playCustomLaunch(currentTable.launchAudio);
                }
            }
        }
    };

    // --- 3. CONDITIONAL RENDERING ---
    if (isLandscape) {
        // EDITOR MODE (Landscape: 40/60 Split View)

        // Start two columns (with a visible border)
        ImGui::Columns(2, "metadata_landscape_split", true);

        // Set the width of the first column (e.g., 40% for info, 60% for media)
        float infoColumnWidth = panelWidth * 0.40f;
        ImGui::SetColumnWidth(0, infoColumnWidth);

        // --- COLUMN 1: INFO ---
        // Use ImGui::BeginChild to allow the info column to scroll independently
        ImGui::BeginChild("metadata_info_scroll", ImVec2(0, -1), false, ImGuiWindowFlags_HorizontalScrollbar);
        DrawInfoContent();
        ImGui::EndChild();

        ImGui::NextColumn();

        // --- COLUMN 2: MEDIA ---
        // Use ImGui::BeginChild to allow the media column to scroll independently
        ImGui::BeginChild("metadata_media_scroll", ImVec2(0, -1), true, ImGuiWindowFlags_HorizontalScrollbar);
        DrawMediaContent();
        ImGui::EndChild();

        ImGui::Columns(1); // End columns
    } else {
        // FRONTEND MODE (Portrait: Simple Stacked Text Only)
        DrawInfoContent();
        // Media content is skipped, as DrawMediaContent is only called inside the isLandscape block.
    }

    ImGui::End();
}
