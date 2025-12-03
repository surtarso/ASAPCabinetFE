#include "metadata_view.h"
#include <filesystem>
#include <cmath>
#include <algorithm>

void MetadataView::render(const TableData& currentTable,
                          int editorWidth,
                          int editorHeight,
                          const Settings& settings,
                          SDL_Renderer* uiRenderer)
{
    float panelWidth  = static_cast<float>(editorWidth);
    float panelHeight = static_cast<float>(editorHeight);
    float posX = 0.0f;
    float posY = 0.0f;

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

auto DrawInfoContent = [&]() {
    std::filesystem::path filePath(currentTable.vpxFile);

    // ------------------------
    // BASIC TABLE SUMMARY
    // ------------------------
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

    // ============================================================================
    //  FILE METADATA (FULL)
    // ============================================================================
    ImGui::Separator();
    ImGui::TextColored(ImVec4(1, 1, 0, 1), "FILE METADATA (VPIN / VPXTOOL)");

    if (!currentTable.tableAuthor.empty())
        ImGui::Text("Author: %s", currentTable.tableAuthor.c_str());
    if (!currentTable.tableDescription.empty())
        ImGui::TextWrapped("Description: %s", currentTable.tableDescription.c_str());
    if (!currentTable.tableSaveDate.empty())
        ImGui::Text("Save Date: %s", currentTable.tableSaveDate.c_str());
    if (!currentTable.tableLastModified.empty())
        ImGui::Text("Last Modified: %s", currentTable.tableLastModified.c_str());
    if (!currentTable.tableReleaseDate.empty())
        ImGui::Text("Release Date: %s", currentTable.tableReleaseDate.c_str());
    if (!currentTable.tableVersion.empty())
        ImGui::Text("Table Version: %s", currentTable.tableVersion.c_str());
    if (!currentTable.tableRevision.empty())
        ImGui::Text("Table Revision: %s", currentTable.tableRevision.c_str());
    if (!currentTable.tableBlurb.empty())
        ImGui::TextWrapped("Blurb: %s", currentTable.tableBlurb.c_str());
    if (!currentTable.tableRules.empty())
        ImGui::TextWrapped("Rules: %s", currentTable.tableRules.c_str());
    if (!currentTable.tableAuthorEmail.empty())
        ImGui::Text("Author Email: %s", currentTable.tableAuthorEmail.c_str());
    if (!currentTable.tableAuthorWebsite.empty())
        ImGui::Text("Author Website: %s", currentTable.tableAuthorWebsite.c_str());
    if (!currentTable.tableRom.empty())
        ImGui::Text("ROM (file metadata): %s", currentTable.tableRom.c_str());

    // Properties sub-fields
    if (!currentTable.tableType.empty())
        ImGui::Text("Type: %s", currentTable.tableType.c_str());
    if (!currentTable.tableManufacturer.empty())
        ImGui::Text("Manufacturer: %s", currentTable.tableManufacturer.c_str());
    if (!currentTable.tableYear.empty())
        ImGui::Text("Year: %s", currentTable.tableYear.c_str());

    // ============================================================================
    // VPSDB METADATA (FULL)
    // ============================================================================
    if (!(
        currentTable.vpsId.empty() &&
        currentTable.vpsName.empty() &&
        currentTable.vpsType.empty() &&
        currentTable.vpsThemes.empty() &&
        currentTable.vpsDesigners.empty() &&
        currentTable.vpsPlayers.empty() &&
        currentTable.vpsIpdbUrl.empty() &&
        currentTable.vpsVersion.empty() &&
        currentTable.vpsAuthors.empty() &&
        currentTable.vpsFeatures.empty() &&
        currentTable.vpsComment.empty() &&
        currentTable.vpsManufacturer.empty() &&
        currentTable.vpsYear.empty() &&
        currentTable.vpsTableImgUrl.empty() &&
        currentTable.vpsTableUrl.empty() &&
        currentTable.vpsB2SImgUrl.empty() &&
        currentTable.vpsB2SUrl.empty() &&
        currentTable.vpsFormat.empty()
    )) {
        ImGui::Separator();
        ImGui::TextColored(ImVec4(1, 1, 0, 1), "VPSDB METADATA");
    }

    if (!currentTable.vpsId.empty())          ImGui::Text("ID: %s", currentTable.vpsId.c_str());
    if (!currentTable.vpsName.empty())        ImGui::Text("Name: %s", currentTable.vpsName.c_str());
    if (!currentTable.vpsType.empty())        ImGui::Text("Type: %s", currentTable.vpsType.c_str());
    if (!currentTable.vpsThemes.empty())      ImGui::Text("Themes: %s", currentTable.vpsThemes.c_str());
    if (!currentTable.vpsDesigners.empty())   ImGui::Text("Designers: %s", currentTable.vpsDesigners.c_str());
    if (!currentTable.vpsPlayers.empty())     ImGui::Text("Players: %s", currentTable.vpsPlayers.c_str());
    if (!currentTable.vpsIpdbUrl.empty())     ImGui::Text("IPDB URL: %s", currentTable.vpsIpdbUrl.c_str());
    if (!currentTable.vpsVersion.empty())     ImGui::Text("Version: %s", currentTable.vpsVersion.c_str());
    if (!currentTable.vpsAuthors.empty())     ImGui::Text("Authors: %s", currentTable.vpsAuthors.c_str());
    if (!currentTable.vpsFeatures.empty())    ImGui::Text("Features: %s", currentTable.vpsFeatures.c_str());
    if (!currentTable.vpsFormat.empty())      ImGui::Text("Format: %s", currentTable.vpsFormat.c_str());
    if (!currentTable.vpsManufacturer.empty())ImGui::Text("Manufacturer: %s", currentTable.vpsManufacturer.c_str());
    if (!currentTable.vpsYear.empty())        ImGui::Text("Year: %s", currentTable.vpsYear.c_str());
    if (!currentTable.vpsComment.empty())     ImGui::TextWrapped("Comment: %s", currentTable.vpsComment.c_str());
    if (!currentTable.vpsTableImgUrl.empty()) ImGui::Text("Table Img URL: %s", currentTable.vpsTableImgUrl.c_str());
    if (!currentTable.vpsTableUrl.empty())    ImGui::Text("Table File URL: %s", currentTable.vpsTableUrl.c_str());
    if (!currentTable.vpsB2SImgUrl.empty())   ImGui::Text("B2S Img URL: %s", currentTable.vpsB2SImgUrl.c_str());
    if (!currentTable.vpsB2SUrl.empty())      ImGui::Text("B2S File URL: %s", currentTable.vpsB2SUrl.c_str());

    // ============================================================================
    // LAUNCHBOX METADATA
    // ============================================================================
    if (!currentTable.flyerFront.empty() ||
        !currentTable.flyerBack.empty() ||
        !currentTable.lbdbID.empty())
    {
        ImGui::Separator();
        ImGui::TextColored(ImVec4(1, 1, 0, 1), "LAUNCHBOX METADATA");
    }

    if (!currentTable.flyerFront.empty())
        ImGui::Text("Flyer Front: %s", currentTable.flyerFront.c_str());
    if (!currentTable.flyerBack.empty())
        ImGui::Text("Flyer Back: %s", currentTable.flyerBack.c_str());
    if (!currentTable.lbdbID.empty())
        ImGui::Text("LBDB ID: %s", currentTable.lbdbID.c_str());

    // ============================================================================
    // OPERATIONAL TAGS (FULL)
    // ============================================================================
    ImGui::Separator();
    ImGui::TextColored(ImVec4(1, 1, 0, 1), "OPERATIONAL TAGS");

    ImGui::Text("Play Count: %d", currentTable.playCount);
    ImGui::Text("Broken: %s", currentTable.isBroken ? "Yes" : "No");
    ImGui::Text("Play Time Last: %.1f sec", currentTable.playTimeLast);
    ImGui::Text("Play Time Total: %.1f sec", currentTable.playTimeTotal);
    ImGui::Text("Folder Last Modified: %llu", (unsigned long long)currentTable.folderLastModified);
    ImGui::Text("File Last Modified: %llu", (unsigned long long)currentTable.fileLastModified);

    // Flags: extra asset markers
    auto addFlag = [&](const char* name, bool flag) {
        if (flag) ImGui::BulletText("%s", name);
    };

    ImGui::Text("Extra Assets:");
    ImGui::Indent();
    addFlag("AltSound",      currentTable.hasAltSound);
    addFlag("AltColor",      currentTable.hasAltColor);
    addFlag("PuP-Pack",      currentTable.hasPup);
    addFlag("AltMusic",      currentTable.hasAltMusic);
    addFlag("UltraDMD",      currentTable.hasUltraDMD);
    addFlag("B2S",           currentTable.hasB2S);
    addFlag("INI",           currentTable.hasINI);
    addFlag("VBS",           currentTable.hasVBS);
    addFlag("Override JSON", currentTable.hasOverride);
    ImGui::Unindent();

    // Script patch info
    if (!currentTable.hashFromVpx.empty() || !currentTable.hashFromVbs.empty()) {
        ImGui::Text("Script Hash (VPX): %s", currentTable.hashFromVpx.c_str());
        ImGui::Text("Script Hash (VBS): %s", currentTable.hashFromVbs.c_str());
    }
    ImGui::Text("Patched: %s", currentTable.isPatched ? "Yes" : "No");
    ImGui::Text("Has different VBS: %s", currentTable.hasDiffVbs ? "Yes" : "No");
};


    auto DrawMediaContent = [&]() {
        ImGui::TextColored(ImVec4(1, 1, 0, 1), "MEDIA PREVIEW");

        auto drawMediaPair = [&](const char* label,
                                 const std::string& imagePath, const std::string& videoPath,
                                 bool hasImage, bool hasVideo,
                                 bool sideBySide = true,
                                int customThumbHeight = -1)
        {
            if (!hasImage && !hasVideo) return;

            ImGui::Separator();
            ImGui::TextColored(ImVec4(0.8f, 0.8f, 1.0f, 1.0f), "%s", label);

            if (uiRenderer == nullptr) {
                if (hasImage) ImGui::Text("[image]");
                if (hasVideo) ImGui::Text("[video]");
                return;
            }

            // Default height for all media
            const int defaultThumbHeight = 250;

            // Use custom value if provided
            const int thumbHeight = (customThumbHeight > 0)
                                        ? customThumbHeight
                                        : defaultThumbHeight;

            const char* imageLabel = "Image:";
            const char* videoLabel = "Video:";

            if (strcmp(label, "Flyer") == 0) {
                imageLabel = "Front:";
                videoLabel = "Back:";
            }

            if (hasImage) {
                if (sideBySide) ImGui::BeginGroup();
                ImGui::Text("%s", imageLabel);

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

                if (sideBySide) ImGui::EndGroup();
            }

            if (hasVideo) {
                if (sideBySide && hasImage) ImGui::SameLine();
                if (sideBySide) ImGui::BeginGroup();

                ImGui::Text("%s", videoLabel);

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

                if (sideBySide) ImGui::EndGroup();
            }
        };

        drawMediaPair("Playfield", currentTable.playfieldImage, currentTable.playfieldVideo,
                      currentTable.hasPlayfieldImage, currentTable.hasPlayfieldVideo, true, 550);

        drawMediaPair("Backglass", currentTable.backglassImage, currentTable.backglassVideo,
                      currentTable.hasBackglassImage, currentTable.hasBackglassVideo, true);

        drawMediaPair("Topper", currentTable.topperImage, currentTable.topperVideo,
                      currentTable.hasTopperImage, currentTable.hasTopperVideo, true);

        drawMediaPair("DMD", currentTable.dmdImage, currentTable.dmdVideo,
                      currentTable.hasDmdImage, currentTable.hasDmdVideo, false);

        drawMediaPair("Flyer", currentTable.flyerFront, currentTable.flyerBack,
                      currentTable.hasFlyerFront, currentTable.hasFlyerBack, true, 450);

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

        if (currentTable.hasTableMusic || currentTable.hasLaunchAudio) {
            ImGui::Separator();
            ImGui::TextColored(ImVec4(1.0f, 0.9f, 0.6f, 1.0f), "AUDIO PREVIEW");

            if (currentTable.hasTableMusic) {
                ImGui::Text("Table Music:");
                ImGui::SameLine();

                if (ImGui::Button("Play##Music")) {
                    if (!currentTable.music.empty() && std::filesystem::exists(currentTable.music))
                        soundManager_->playTableMusic(currentTable.music);
                }
                ImGui::SameLine();
                if (ImGui::Button("Stop##Music")) {
                    soundManager_->stopMusic();
                }
            }

            if (currentTable.hasLaunchAudio) {
                ImGui::Text("Launch Audio:");
                ImGui::SameLine();

                if (ImGui::Button("Play##Launch")) {
                    if (!currentTable.launchAudio.empty() && std::filesystem::exists(currentTable.launchAudio))
                        soundManager_->playCustomLaunch(currentTable.launchAudio);
                }
            }
        }
    };

    ImGui::Columns(2, "metadata_landscape_split", true);

    float infoColumnWidth = panelWidth * 0.40f;
    ImGui::SetColumnWidth(0, infoColumnWidth);

    ImGui::BeginChild("metadata_info_scroll", ImVec2(0, -1), false, ImGuiWindowFlags_HorizontalScrollbar);
    DrawInfoContent();
    ImGui::EndChild();

    ImGui::NextColumn();

    ImGui::BeginChild("metadata_media_scroll", ImVec2(0, -1), true, ImGuiWindowFlags_HorizontalScrollbar);
    DrawMediaContent();
    ImGui::EndChild();

    ImGui::Columns(1);

    ImGui::End();
}
