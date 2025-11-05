#include "loading_screen.h"
#include "imgui.h"
#include <SDL2/SDL.h>
#include <algorithm>
#include <cmath>
#include <string>
#include <vector>
#include <cstdio>

LoadingScreen::LoadingScreen(std::shared_ptr<LoadingProgress> progress)
    : loadingProgress_(progress) {
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

    // Calculate dynamic window and section heights
    float minTopperContentHeight = 3 * textLineHeightWithSpacing;
    float minTopperHeight = minTopperContentHeight + framePaddingY * 4;

    // Backglass: "Pipeline:" label + 10 pipeline stages + 3 sets of (label + progress bar)
    float minBackglassContentHeight = (1 + 10) * textLineHeightWithSpacing + 3 * textLineHeight + 3 * 20.0f;
    float minBackglassHeight = minBackglassContentHeight + framePaddingY * 4;

    float minDmdContentHeight = 3 * textLineHeightWithSpacing;
    float minDmdHeight = minDmdContentHeight + framePaddingY * 4;

    float minPlayfieldContentHeight = 1 * textLineHeightWithSpacing + loadingProgress_->maxLogMessages * textLineHeightWithSpacing;
    float minPlayfieldHeight = minPlayfieldContentHeight + framePaddingY * 4;

    float totalMinRequiredHeight = minTopperHeight + minBackglassHeight + minDmdHeight + minPlayfieldHeight + itemSpacingY * 3 + windowPaddingY * 2;

    // Set window size
    float windowWidth = std::min(io.DisplaySize.x * 0.45f, 550.0f);
    float windowHeight = std::min(io.DisplaySize.y * 0.95f, std::max(totalMinRequiredHeight, 600.0f));

    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(windowWidth, windowHeight), ImGuiCond_Always);
    ImGui::Begin("Loading", nullptr, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings);

    std::lock_guard<std::mutex> lock(loadingProgress_->mutex);

    // Allocate child heights dynamically
    float availableHeight = ImGui::GetContentRegionAvail().y;
    float topperAllocHeight = minTopperHeight;
    float backglassAllocHeight = minBackglassHeight;
    float dmdAllocHeight = minDmdHeight;
    float playfieldAllocHeight = availableHeight - topperAllocHeight - backglassAllocHeight - dmdAllocHeight - itemSpacingY * 3;
    if (playfieldAllocHeight < minPlayfieldHeight) playfieldAllocHeight = minPlayfieldHeight;

    // 1. Topper (System Information - Centered)
    ImGui::BeginChild("Topper", ImVec2(-1, topperAllocHeight), true, ImGuiWindowFlags_NoScrollbar);
    ImGui::PushStyleColor(ImGuiCol_Text, colorNeonCyan_);
    float startYTopper = (ImGui::GetWindowHeight() - minTopperContentHeight) * 0.5f;
    if (startYTopper < 0) startYTopper = 0;
    ImGui::SetCursorPosY(startYTopper);

    auto centerTextLine = [](const char* text) {
        float textWidth = ImGui::CalcTextSize(text).x;
        ImGui::SetCursorPosX((ImGui::GetWindowWidth() - textWidth) * 0.5f);
        ImGui::Text("%s", text);
    };

    centerTextLine(("Kernel: " + systemInfo_.kernel).c_str());
    centerTextLine(("CPU: " + systemInfo_.cpuModel).c_str());
    centerTextLine(("RAM: " + systemInfo_.totalRam + " MB").c_str());
    ImGui::PopStyleColor();
    ImGui::EndChild();

    ImGui::Spacing();

    // 2. Backglass (Pipeline Stages & Progress Bars)
    ImGui::BeginChild("Backglass", ImVec2(-1, backglassAllocHeight), true, ImGuiWindowFlags_NoScrollbar);
    ImGui::Columns(2, "BackglassColumns", false);
    ImGui::SetColumnWidth(0, ImGui::GetWindowWidth() * 0.45f);

    // Left Column: Pipeline Stages (Vertical)
    ImGui::Text("Pipeline:");
    const char* stages[] = {
        "Loading Index",
        "Scanning VPX Files",
        "Merging Index",
        "Scanning Metadata",
        "Matching VPSDB",
        "Saving Index",
        "Downloading Media",
        "Patching Tables",
        "Applying Overrides",
        "Sorting Tables",
        "Loading Complete"
    };

    for (int i = 0; i < loadingProgress_->totalStages; ++i) {
        float alpha = (i == loadingProgress_->currentStage - 1) ? (0.8f + 0.2f * std::sin(ImGui::GetTime() * 2.0f)) : (i < loadingProgress_->currentStage ? 0.8f : 0.3f);
        ImGui::TextColored(ImVec4(colorTimelineNode_.x, colorTimelineNode_.y, colorTimelineNode_.z, alpha), "%s", stages[i]);
    }
    ImGui::NextColumn();

    // Right Column: Progress Bars
    float overallProgress = loadingProgress_->totalStages > 0 ? static_cast<float>(loadingProgress_->currentStage) / static_cast<float>(loadingProgress_->totalStages) : 0.0f;
    ImGui::Text("Overall: %.0f%%", overallProgress * 100.0f);
    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, colorNeonCyan_);
    ImGui::ProgressBar(overallProgress, ImVec2(-1, 20));
    ImGui::PopStyleColor();

    float tableProgress = loadingProgress_->totalTablesToLoad > 0 ? static_cast<float>(loadingProgress_->currentTablesLoaded) / static_cast<float>(loadingProgress_->totalTablesToLoad) : 0.0f;
    ImGui::Text("Tables: %zu/%zu (%.0f%%)", loadingProgress_->currentTablesLoaded, loadingProgress_->totalTablesToLoad, tableProgress * 100.0f);
    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, colorNeonMagenta_);
    ImGui::ProgressBar(tableProgress, ImVec2(-1, 20));
    ImGui::PopStyleColor();

    // Use stage-specific denominator for matches
    size_t matchDenominator = (loadingProgress_->currentStage == 5) ? loadingProgress_->totalTablesToLoad : static_cast<size_t>(loadingProgress_->numMatched + loadingProgress_->numNoMatch);
    float matchedProgress = matchDenominator > 0 ? static_cast<float>(loadingProgress_->numMatched) / static_cast<float>(matchDenominator) : 0.0f;
    ImGui::Text("Matches: %d/%zu (%.0f%%)", loadingProgress_->numMatched, matchDenominator, matchedProgress * 100.0f);
    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, colorNeonYellow_);
    ImGui::ProgressBar(matchedProgress, ImVec2(-1, 20));
    ImGui::PopStyleColor();

    ImGui::Columns(1);
    ImGui::EndChild();

    ImGui::Spacing();

    // 3. DMD (Score/Stats - Centered)
    ImGui::BeginChild("DMD", ImVec2(-1, dmdAllocHeight), true, ImGuiWindowFlags_NoScrollbar);
    ImGui::PushStyleColor(ImGuiCol_Text, colorDmdText_);
    float textBlockHeightDMD = 4 * textLineHeightWithSpacing; // Added line for current task
    float startYDMD = (ImGui::GetWindowHeight() - textBlockHeightDMD) * 0.5f;
    if (startYDMD < 0) startYDMD = 0;
    ImGui::SetCursorPosY(startYDMD);

    centerTextLine(("Task: " + loadingProgress_->currentTask).c_str());
    centerTextLine(("Tables: " + std::to_string(loadingProgress_->totalTablesToLoad)).c_str());
    centerTextLine(("Matched: " + std::to_string(loadingProgress_->numMatched)).c_str());
    centerTextLine(("Unmatched: " + std::to_string(loadingProgress_->numNoMatch)).c_str());
    ImGui::PopStyleColor();
    ImGui::EndChild();

    ImGui::Spacing();

    // 4. Playfield (Logs only - Fills remaining space, no scrollbar)
    ImGui::BeginChild("Playfield", ImVec2(-1, playfieldAllocHeight), true, ImGuiWindowFlags_NoScrollbar);
    ImGui::Text("Logs:");
    ImGui::BeginChild("LogTerminal", ImVec2(-1, -1), true, ImGuiWindowFlags_NoScrollbar);
    ImGui::PushStyleColor(ImGuiCol_Text, colorDmdText_);
    for (const auto& msg : loadingProgress_->logMessages) {
        // Skip debug messages (those starting with "DEBUG:")
        if (msg.compare(0, 6, "DEBUG:") == 0) continue;
        std::string displayMsg = msg;
        // Strip INFO: or ERROR: prefixes for cleaner display
        if (displayMsg.compare(0, 6, "INFO:") == 0 || displayMsg.compare(0, 7, "ERROR:") == 0) {
            size_t prefixEnd = displayMsg.find(':') + 1;
            size_t firstNonSpace = displayMsg.find_first_not_of(" \t", prefixEnd);
            if (firstNonSpace != std::string::npos) displayMsg = displayMsg.substr(firstNonSpace);
            else displayMsg = "";
        }
        if (displayMsg.empty()) continue;
        if (displayMsg.find("Matched ") == 0) {
            ImGui::TextColored(ImVec4(0.2f, 0.8f, 0.2f, 1.0f), "%s", displayMsg.c_str());
        } else if (displayMsg.find("No match for ") == 0 || displayMsg.find("Failed") != std::string::npos) {
            ImGui::TextColored(ImVec4(1.0f, 0.2f, 0.2f, 1.0f), "%s", displayMsg.c_str());
        } else {
            ImGui::Text("%s", displayMsg.c_str());
        }
    }
    ImGui::PopStyleColor();
    ImGui::EndChild();
    ImGui::EndChild();
    ImGui::End();
}
