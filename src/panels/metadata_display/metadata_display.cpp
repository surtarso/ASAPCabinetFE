#include "panels/metadata_display/metadata_display.h"
#include <SDL_image.h>
#include <algorithm>
#include <cmath>
#include <filesystem>

void MetadataDisplay::render(const TableData &currentTable,
                             int playfieldWidth,
                             int playfieldHeight,
                             const Settings &settings,
                             SDL_Renderer *uiRenderer)

{
    float panelWidth = static_cast<float>(playfieldWidth) * settings.metadataPanelWidth;
    float panelHeight = static_cast<float>(playfieldHeight) * settings.metadataPanelHeight;
    float posX = (static_cast<float>(playfieldWidth) - panelWidth) / 2.0f;
    float posY = (static_cast<float>(playfieldHeight) - panelHeight) / 2.0f;

    ImGui::SetNextWindowPos(ImVec2(posX, posY));
    ImGui::SetNextWindowSize(ImVec2(panelWidth, panelHeight));
    ImGui::SetNextWindowBgAlpha(settings.metadataPanelAlpha);

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
                             ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoMove |
                             ImGuiWindowFlags_NoResize;

    bool isOpen = ImGui::Begin("Table Metadata", nullptr, flags);

    if (!isOpen)
    {
        if (wasOpen_) {
            if (flyerFrontTex_) {
                SDL_DestroyTexture(flyerFrontTex_);
                flyerFrontTex_ = nullptr;
            }
            if (flyerBackTex_) {
                SDL_DestroyTexture(flyerBackTex_);
                flyerBackTex_ = nullptr;
            }
            lastFlyerFront_ = "";
            lastFlyerBack_ = "";
        }
        wasOpen_ = false;
        ImGui::End();
        return;
    }

    wasOpen_ = true;

    auto DrawInfoContent = [&]()
    {
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
        bool hasYear = !currentTable.year.empty();
        if (hasManuf && hasYear)
            ImGui::Text("Manufacturer / Year: %s / %s", currentTable.manufacturer.c_str(), currentTable.year.c_str());
        else if (hasManuf)
            ImGui::Text("Manufacturer: %s", currentTable.manufacturer.c_str());
        else if (hasYear)
            ImGui::Text("Year: %s", currentTable.year.c_str());

        if (currentTable.matchConfidence > 0)
        {
            int fullStars = static_cast<int>(std::round(currentTable.matchConfidence * 10.0f));
            fullStars = std::clamp(fullStars, 0, 10);
            ImGui::Text("Match Confidence:");
            ImGui::SameLine();
            ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 255, 0, 255));
            for (int i = 0; i < fullStars; ++i)
            {
                ImGui::TextUnformatted("+");
                ImGui::SameLine();
            }
            ImGui::PopStyleColor();
            for (int i = fullStars; i < 10; ++i)
            {
                ImGui::TextUnformatted("-");
                ImGui::SameLine();
            }
            ImGui::NewLine();
        }
        ImGui::Text("Source: %s", currentTable.jsonOwner.c_str());

        if (!currentTable.vpsId.empty() || !currentTable.vpsManufacturer.empty() ||
            !currentTable.vpsYear.empty() || !currentTable.vpsType.empty() ||
            !currentTable.vpsThemes.empty() || !currentTable.vpsDesigners.empty() ||
            !currentTable.vpsPlayers.empty() || !currentTable.vpsIpdbUrl.empty() ||
            !currentTable.vpsVersion.empty() || !currentTable.vpsAuthors.empty() ||
            !currentTable.vpsFeatures.empty() || !currentTable.vpsComment.empty() ||
            !currentTable.vpsFormat.empty())
        {
            ImGui::Separator();
            ImGui::TextColored(ImVec4(1, 1, 0, 1), "VPSDB DETAILS");
        }

        if (!currentTable.vpsId.empty())
            ImGui::Text("ID: %s", currentTable.vpsId.c_str());
        if (!currentTable.vpsManufacturer.empty())
            ImGui::Text("Manufacturer: %s", currentTable.vpsManufacturer.c_str());
        if (!currentTable.vpsYear.empty())
            ImGui::Text("Year: %s", currentTable.vpsYear.c_str());
        if (!currentTable.vpsType.empty())
            ImGui::Text("Type: %s", currentTable.vpsType.c_str());
        if (!currentTable.vpsThemes.empty())
            ImGui::Text("Themes: %s", currentTable.vpsThemes.c_str());
        if (!currentTable.vpsDesigners.empty())
            ImGui::Text("Designers: %s", currentTable.vpsDesigners.c_str());
        if (!currentTable.vpsPlayers.empty())
            ImGui::Text("Players: %s", currentTable.vpsPlayers.c_str());
        if (!currentTable.vpsIpdbUrl.empty())
            ImGui::Text("IPDB URL: %s", currentTable.vpsIpdbUrl.c_str());
        if (!currentTable.vpsVersion.empty())
            ImGui::Text("Version: %s", currentTable.vpsVersion.c_str());
        if (!currentTable.vpsAuthors.empty())
            ImGui::Text("Authors: %s", currentTable.vpsAuthors.c_str());
        if (!currentTable.vpsFeatures.empty())
            ImGui::Text("Features: %s", currentTable.vpsFeatures.c_str());
        if (!currentTable.vpsFormat.empty())
            ImGui::Text("Format: %s", currentTable.vpsFormat.c_str());
        if (!currentTable.vpsComment.empty())
            ImGui::TextWrapped("Comment: %s", currentTable.vpsComment.c_str());
    };

    DrawInfoContent();
    if (uiRenderer && (currentTable.hasFlyerFront || currentTable.hasFlyerBack))
    {
        ImGui::Separator();
        ImGui::TextColored(ImVec4(0.8f, 0.8f, 1.0f, 1.0f), "Flyer");
        const int maxHeight = static_cast<int>(panelWidth * 0.62f);
        ImGui::BeginGroup();
        if (currentTable.hasFlyerFront)
        {
            if (flyerFrontTex_ == nullptr || currentTable.flyerFront != lastFlyerFront_) {
                if (flyerFrontTex_) SDL_DestroyTexture(flyerFrontTex_);
                flyerFrontTex_ = IMG_LoadTexture(uiRenderer, currentTable.flyerFront.c_str());
                lastFlyerFront_ = currentTable.flyerFront;
            }
            ImGui::BeginGroup();
            ImGui::Text("Front:");
            if (flyerFrontTex_)
            {
                int w = 0, h = 0;
                SDL_QueryTexture(flyerFrontTex_, nullptr, nullptr, &w, &h);
                float scale = static_cast<float>(maxHeight) / static_cast<float>(h);
                ImGui::Image((void *)flyerFrontTex_, ImVec2(static_cast<float>(w) * scale, static_cast<float>(h) * scale));
            }
            else
            {
                ImGui::TextColored(ImVec4(1, 0.3f, 0.3f, 1), "Failed to load image");
            }
            ImGui::EndGroup();
        }
        if (currentTable.hasFlyerBack)
        {
            if (currentTable.hasFlyerFront)
                ImGui::SameLine();
            if (flyerBackTex_ == nullptr || currentTable.flyerBack != lastFlyerBack_) {
                if (flyerBackTex_) SDL_DestroyTexture(flyerBackTex_);
                flyerBackTex_ = IMG_LoadTexture(uiRenderer, currentTable.flyerBack.c_str());
                lastFlyerBack_ = currentTable.flyerBack;
            }
            ImGui::BeginGroup();
            ImGui::Text("Back:");
            if (flyerBackTex_)
            {
                int w = 0, h = 0;
                SDL_QueryTexture(flyerBackTex_, nullptr, nullptr, &w, &h);
                float scale = static_cast<float>(maxHeight) / static_cast<float>(h);
                ImGui::Image((void *)flyerBackTex_, ImVec2(static_cast<float>(w) * scale, static_cast<float>(h) * scale));
            }
            else
            {
                ImGui::TextColored(ImVec4(1, 0.3f, 0.3f, 1), "Failed to load image");
            }
            ImGui::EndGroup();
        }
        ImGui::EndGroup();
    }
    ImGui::End();
}
