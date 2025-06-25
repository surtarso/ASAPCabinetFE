#include "playfield_overlay.h"
#include "metadata_panel.h"
#include "config/settings.h"
#include "log/logging.h"
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
        MetadataPanel metadataPanel;
        metadataPanel.render(tables_->at(*currentIndex_), playfieldWidth, playfieldHeight, configService_->getSettings());
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