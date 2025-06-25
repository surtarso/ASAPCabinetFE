#ifndef LOADING_SCREEN_H
#define LOADING_SCREEN_H

#include <memory>
#include <string>
#include <SDL2/SDL.h>
#include "imgui.h"
#include "loading_progress.h"

/**
 * @class LoadingScreen
 * @brief Manages the ImGui-based loading screen for ASAPCabinetFE initialization.
 *
 * Renders a pinball-themed loading interface with a vertical pipeline stage list,
 * progress bars, system information, log messages, and simulated thread activity.
 * The layout is dynamically sized (70% width/50% height, capped at 800x600) with
 * neon colors and lightweight animations, designed for vertical screens. No user
 * interaction is required—everything is visible like a dashboard.
 */
class LoadingScreen {
public:
    LoadingScreen(std::shared_ptr<LoadingProgress> progress);
    void render();

private:
    std::shared_ptr<LoadingProgress> loadingProgress_;
    // float lastTablesLoaded_ = 0.0f;
    // float lastNumMatched_ = 0.0f;
    static constexpr size_t maxThreads_ = 6;
    static constexpr ImVec4 colorNeonCyan_ = ImVec4(0.0f, 1.0f, 1.0f, 1.0f);
    static constexpr ImVec4 colorNeonMagenta_ = ImVec4(1.0f, 0.0f, 1.0f, 1.0f);
    static constexpr ImVec4 colorNeonYellow_ = ImVec4(1.0f, 1.0f, 0.0f, 1.0f);
    static constexpr ImVec4 colorDmdText_ = ImVec4(1.0f, 0.8f, 0.2f, 1.0f);
    static constexpr ImVec4 colorTimelineNode_ = ImVec4(0.2f, 0.8f, 1.0f, 1.0f);
    static constexpr ImVec4 colorThreadGreen_ = ImVec4(0.2f, 0.8f, 0.2f, 0.8f);

    struct SystemInfo {
        std::string kernel;
        std::string cpuModel;
        std::string totalRam;
    } systemInfo_;

    std::string getCommandOutput(const std::string& cmd);
};

#endif // LOADING_SCREEN_H