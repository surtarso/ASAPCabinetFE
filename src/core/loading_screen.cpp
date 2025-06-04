#include "core/loading_screen.h"
#include "imgui.h"
#include <SDL2/SDL.h> // Required for SDL_GetDesktopDisplayMode if used for window sizing
#include <algorithm> // For std::min/max
#include <cmath> // For std::sin
#include <string> // For std::string manipulation

LoadingScreen::LoadingScreen(std::shared_ptr<LoadingProgress> progress)
    : loadingProgress_(progress) {
}

void LoadingScreen::render() {
    ImGuiIO& io = ImGui::GetIO();
    // Calculate a good size relative to display, or fixed if preferred
    float windowWidth = std::min(io.DisplaySize.x * 0.5f, 600.0f); // Max 600, or 50% of display
    float windowHeight = std::min(io.DisplaySize.y * 0.7f, 400.0f); // Increased height to accommodate three bars

    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(windowWidth, windowHeight), ImGuiCond_Always);
    ImGui::Begin("Loading", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings);

    // Lock the progress data for reading
    std::lock_guard<std::mutex> lock(loadingProgress_->mutex);

    ImGui::Text("ASAPCabinetFE tables are now loading.");
    ImGui::NewLine();

    // Overall Progress Bar with Fade Animation (Deep Blue - Nebula)
    float overallProgress = 0.0f;
    if (loadingProgress_->totalStages > 0) {
        overallProgress = (float)loadingProgress_->currentStage / loadingProgress_->totalStages;
    }
    float time = ImGui::GetTime();
    float fadeAlpha = 0.7f + 0.3f * (std::sin(time * 3.0f) * 0.5f + 0.5f); // Range: 0.7 to 1.0, faster at 3 rad/s
    char overlayText[16];
    snprintf(overlayText, sizeof(overlayText), "%.0f%%", overallProgress * 100.0f);
    ImGui::Text("Overall Progress:");
    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.1f, 0.2f, 0.6f, 1.0f)); // Deep blue
    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, fadeAlpha);
    ImGui::ProgressBar(overallProgress, ImVec2(-1, 0), overlayText);
    ImGui::PopStyleVar();
    ImGui::PopStyleColor();

    // Animated Current Task
    std::string progressText = loadingProgress_->currentTask + " (" + std::to_string(loadingProgress_->currentTablesLoaded) + ")";
    ImGui::Text("%s", progressText.c_str());

    // Per-Table Progress Bar (Purple - Galaxy Core)
    float tableProgress = 0.0f;
    if (loadingProgress_->totalTablesToLoad > 0) {
        tableProgress = (float)loadingProgress_->currentTablesLoaded / loadingProgress_->totalTablesToLoad;
    }
    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.5f, 0.2f, 0.8f, 1.0f)); // Purple
    ImGui::ProgressBar(tableProgress, ImVec2(-1, 0));
    ImGui::PopStyleColor();

    // Matched Progress Bar (Pinkish-White - Star Glow)
    float matchedProgress = 0.0f;
    if (loadingProgress_->totalTablesToLoad > 0) {
        matchedProgress = (float)loadingProgress_->numMatched / loadingProgress_->totalTablesToLoad;
    }
    char matchedOverlayText[16];
    snprintf(matchedOverlayText, sizeof(matchedOverlayText), "%.0f%%", matchedProgress * 100.0f);
    ImGui::Text("Matched Progress:");
    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(1.0f, 0.7f, 0.8f, 1.0f)); // Pinkish-white
    ImGui::ProgressBar(matchedProgress, ImVec2(-1, 0), matchedOverlayText);
    ImGui::PopStyleColor();

    // Stats
    ImGui::Text("Total Matched: %d/%d", loadingProgress_->numMatched, loadingProgress_->totalTablesToLoad);
    ImGui::Text("No Match: %d", loadingProgress_->numNoMatch);
    ImGui::NewLine();

    // Mini Terminal/Log Display
    ImGui::Text("Recent Log:");
    ImGui::BeginChild("LogTerminal", ImVec2(-1, ImGui::GetContentRegionAvail().y), true, ImGuiWindowFlags_HorizontalScrollbar);
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8f, 0.8f, 0.2f, 1.0f)); // Faded yellow
    for (const auto& msg : loadingProgress_->logMessages) {
        // Remove "DEBUG:" from the message
        std::string displayMsg = msg;
        const std::string debugPrefix = "DEBUG:";
        if (displayMsg.compare(0, debugPrefix.length(), debugPrefix) == 0) {
            displayMsg = displayMsg.substr(debugPrefix.length());
            // Remove leading whitespace if any
            size_t firstNonSpace = displayMsg.find_first_not_of(" \t");
            if (firstNonSpace != std::string::npos) {
                displayMsg = displayMsg.substr(firstNonSpace);
            } else {
                displayMsg = "";
            }
        }
        if (!displayMsg.empty()) {
            ImGui::TextUnformatted(displayMsg.c_str());
        }
    }
    ImGui::PopStyleColor();
    // Auto-scroll to bottom only if user hasn't manually scrolled up
    if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
        ImGui::SetScrollHereY(1.0f);
    }
    ImGui::EndChild();

    ImGui::End();
}