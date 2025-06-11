#include "core/loading_screen.h"
#include "imgui.h"
#include <SDL2/SDL.h>
#include <algorithm>
#include <cmath>
#include <string>
#include <vector>
#include <cstdio> // For popen, pclose, fgets

LoadingScreen::LoadingScreen(std::shared_ptr<LoadingProgress> progress)
    : loadingProgress_(progress) {//, lastTablesLoaded_(0.0f), lastNumMatched_(0.0f) {
    // Fetch system info once at startup
    systemInfo_.kernel = getCommandOutput("uname -r");
    systemInfo_.cpuModel = getCommandOutput("cat /proc/cpuinfo | grep 'model name' | uniq | cut -d ':' -f2");
    systemInfo_.totalRam = getCommandOutput("free -m | grep Mem: | awk '{print $2}'");
}

std::string LoadingScreen::getCommandOutput(const std::string& cmd) {
    std::string result;
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) return "N/A";
    char buffer[128];
    while (!feof(pipe)) {
        if (fgets(buffer, 128, pipe) != nullptr) result += buffer;
    }
    pclose(pipe);
    // Remove trailing newline if present
    if (!result.empty() && result.back() == '\n') {
        result.pop_back();
    }
    return result.empty() ? "N/A" : result;
}

void LoadingScreen::render() {
    ImGuiIO& io = ImGui::GetIO();
    const ImGuiStyle& style = ImGui::GetStyle();

    // Get common ImGui style metrics
    float textLineHeight = ImGui::GetTextLineHeight();
    float textLineHeightWithSpacing = ImGui::GetTextLineHeightWithSpacing();
    float framePaddingY = style.FramePadding.y;
    float itemSpacingY = style.ItemSpacing.y;
    float windowPaddingY = style.WindowPadding.y;

    // --- Calculate dynamic window and section heights ---
    // Minimum heights for each section based on content lines and static elements
    // Topper: 3 lines of system info
    float minTopperContentHeight = 3 * textLineHeightWithSpacing;
    float minTopperHeight = minTopperContentHeight + framePaddingY * 4; // Add vertical padding

    // Backglass: "Pipeline:" label + 5 pipeline stages + 3 sets of (label + progress bar)
    float minBackglassContentHeight = (1 + 3) * textLineHeightWithSpacing + 3 * textLineHeight + 3 * 20.0f; // 20.0f is approx progress bar height
    float minBackglassHeight = minBackglassContentHeight + framePaddingY * 4; // Add vertical padding

    // DMD: 3 lines of score info
    float minDmdContentHeight = 3 * textLineHeightWithSpacing;
    float minDmdHeight = minDmdContentHeight + framePaddingY * 4; // Add vertical padding

    // Playfield (Logs): "Logs:" label + maxLogMessages lines
    float minPlayfieldContentHeight = 1 * textLineHeightWithSpacing + loadingProgress_->maxLogMessages * textLineHeightWithSpacing;
    float minPlayfieldHeight = minPlayfieldContentHeight + framePaddingY * 4; // Add vertical padding


    // Total minimum required height for all content + spacing between children
    float totalMinRequiredHeight = minTopperHeight + minBackglassHeight + minDmdHeight + minPlayfieldHeight + itemSpacingY * 3 + windowPaddingY * 2;

    // Set window size
    float windowWidth = std::min(io.DisplaySize.x * 0.45f, 550.0f); // Keep narrower width
    float windowHeight = std::min(io.DisplaySize.y * 0.95f, totalMinRequiredHeight); // Ensure it's tall enough for all content
    if (windowHeight < 600.0f) windowHeight = 600.0f; // Ensure a reasonable minimum if content is minimal

    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(windowWidth, windowHeight), ImGuiCond_Always);
    ImGui::Begin("Loading", nullptr, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings);

    std::lock_guard<std::mutex> lock(loadingProgress_->mutex);

    // --- Allocate child heights dynamically to ensure they fit perfectly ---
    // We'll use remaining space for the playfield
    // float currentYOffset = ImGui::GetCursorPosY(); // Initial Y position in the main window
    float availableHeight = ImGui::GetContentRegionAvail().y; // Total available height for children

    // Calculate heights, ensuring minimums are met and playfield takes the rest
    float topperAllocHeight = minTopperHeight;
    float backglassAllocHeight = minBackglassHeight;
    float dmdAllocHeight = minDmdHeight;

    // Remaining height for playfield after fixed sections and their spacing
    float playfieldAllocHeight = availableHeight - topperAllocHeight - backglassAllocHeight - dmdAllocHeight - itemSpacingY * 3;
    if (playfieldAllocHeight < minPlayfieldHeight) playfieldAllocHeight = minPlayfieldHeight; // Ensure playfield minimum

    // --- 1. Topper (System Information - Centered) ---
    ImGui::BeginChild("Topper", ImVec2(-1, topperAllocHeight), true, ImGuiWindowFlags_NoScrollbar);
    ImGui::PushStyleColor(ImGuiCol_Text, colorNeonCyan_);
    // Calculate starting Y position for vertical centering of the text block
    float startYTopper = (ImGui::GetWindowHeight() - minTopperContentHeight) * 0.5f;
    if (startYTopper < 0) startYTopper = 0; // Prevent negative Y
    ImGui::SetCursorPosY(startYTopper);

    // Centering each line horizontally
    auto centerTextLine = [](const char* text) {
        float textWidth = ImGui::CalcTextSize(text).x;
        ImGui::SetCursorPosX((ImGui::GetWindowWidth() - textWidth) * 0.5f);
        ImGui::Text("%s", text);
    };

    centerTextLine(systemInfo_.kernel.c_str());
    centerTextLine(systemInfo_.cpuModel.c_str());
    centerTextLine(std::string("RAM: " + systemInfo_.totalRam + " MB").c_str());
    ImGui::PopStyleColor();
    ImGui::EndChild(); // End Topper

    // Add spacing between children
    ImGui::Spacing();

    // --- 2. Backglass (Pipeline Stages & Progress Bars) ---
    ImGui::BeginChild("Backglass", ImVec2(-1, backglassAllocHeight), true, ImGuiWindowFlags_NoScrollbar);
    ImGui::Columns(2, "BackglassColumns", false);
    ImGui::SetColumnWidth(0, ImGui::GetWindowWidth() * 0.45f); // Adjust width for pipeline on the left

    // Left Column: Pipeline Stages (Vertical)
    ImGui::Text("Pipeline:");
    const char* stages[] = {"Fetching VPSDB", "Scanning VPX", "Enriching", "Saving Index", "Sorting"};
    for (int i = 0; i < loadingProgress_->totalStages; ++i) {
        float alpha = (i == loadingProgress_->currentStage) ? (0.8f + 0.2f * std::sin(ImGui::GetTime() * 2.0f)) : (i < loadingProgress_->currentStage ? 0.8f : 0.3f);
        ImGui::TextColored(ImVec4(colorTimelineNode_.x, colorTimelineNode_.y, colorTimelineNode_.z, alpha), "%s", stages[i]);
    }
    ImGui::NextColumn();

    // Right Column: Progress Bars
    float overallProgress = loadingProgress_->totalStages > 0 ? (float)loadingProgress_->currentStage / loadingProgress_->totalStages : 0.0f;
    ImGui::Text("Overall: %.0f%%", overallProgress * 100.0f);
    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, colorNeonCyan_);
    ImGui::ProgressBar(overallProgress, ImVec2(-1, 20));
    ImGui::PopStyleColor();

    float tableProgress = loadingProgress_->totalTablesToLoad > 0 ? (float)loadingProgress_->currentTablesLoaded / loadingProgress_->totalTablesToLoad : 0.0f;
    ImGui::Text("Tables: %.0f%%", tableProgress * 100.0f);
    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, colorNeonMagenta_);
    ImGui::ProgressBar(tableProgress, ImVec2(-1, 20));
    ImGui::PopStyleColor();

    float matchedProgress = loadingProgress_->totalTablesToLoad > 0 ? (float)loadingProgress_->numMatched / loadingProgress_->totalTablesToLoad : 0.0f;
    ImGui::Text("Matches: %.0f%%", matchedProgress * 100.0f);
    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, colorNeonYellow_);
    ImGui::ProgressBar(matchedProgress, ImVec2(-1, 20));
    ImGui::PopStyleColor();

    ImGui::Columns(1); // End columns
    ImGui::EndChild(); // End Backglass

    // Add spacing between children
    ImGui::Spacing();

    // --- 3. DMD (Score/Stats - Centered) ---
    ImGui::BeginChild("DMD", ImVec2(-1, dmdAllocHeight), true, ImGuiWindowFlags_NoScrollbar);
    ImGui::PushStyleColor(ImGuiCol_Text, colorDmdText_);
    // Calculate total text block height
    float textBlockHeightDMD = 3 * textLineHeightWithSpacing;
    // Calculate starting Y position for vertical centering
    float startYDMD = (ImGui::GetWindowHeight() - textBlockHeightDMD) * 0.5f;
    if (startYDMD < 0) startYDMD = 0;
    ImGui::SetCursorPosY(startYDMD);

    // Centering each line horizontally
    centerTextLine(std::string("Tables: " + std::to_string(loadingProgress_->totalTablesToLoad)).c_str());
    centerTextLine(std::string("Matched: " + std::to_string(loadingProgress_->numMatched)).c_str());
    centerTextLine(std::string("Unmatched: " + std::to_string(loadingProgress_->numNoMatch)).c_str());
    ImGui::PopStyleColor();
    ImGui::EndChild(); // End DMD

    // Add spacing between children
    ImGui::Spacing();

    // --- 4. Playfield (Logs only - Fills remaining space, no scrollbar) ---
    ImGui::BeginChild("Playfield", ImVec2(-1, playfieldAllocHeight), true, ImGuiWindowFlags_NoScrollbar);
    ImGui::Text("Logs:");
    // The LogTerminal child should now take all available space within the Playfield
    // Use -1 for height to make it fill the rest of the parent child.
    ImGui::BeginChild("LogTerminal", ImVec2(-1, -1), true, ImGuiWindowFlags_NoScrollbar); // Fills parent, no scrollbar
    ImGui::PushStyleColor(ImGuiCol_Text, colorDmdText_); // Using DMD text style for logs
    for (const auto& msg : loadingProgress_->logMessages) {
        std::string displayMsg = msg;
        const std::string debugPrefix = "DEBUG:";
        if (displayMsg.compare(0, debugPrefix.length(), debugPrefix) == 0) {
            displayMsg = displayMsg.substr(debugPrefix.length());
            size_t firstNonSpace = displayMsg.find_first_not_of(" \t");
            if (firstNonSpace != std::string::npos) displayMsg = displayMsg.substr(firstNonSpace);
            else displayMsg = "";
        }
        if (displayMsg.empty()) continue; // Skip empty messages after stripping prefix
        if (displayMsg.find("Processed:") != std::string::npos) {
            ImGui::TextColored(ImVec4(0.2f, 0.8f, 0.2f, 1.0f), "%s", displayMsg.c_str());
        } else if (displayMsg.find("error:") != std::string::npos || displayMsg.find("Failed") != std::string::npos) {
            ImGui::TextColored(ImVec4(1.0f, 0.2f, 0.2f, 1.0f), "%s", displayMsg.c_str());
        } else {
            ImGui::Text("%s", displayMsg.c_str());
        }
    }
    ImGui::PopStyleColor();
    ImGui::EndChild(); // End LogTerminal

    ImGui::EndChild(); // End Playfield
    ImGui::End(); // End main window
}