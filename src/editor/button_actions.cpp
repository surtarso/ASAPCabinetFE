#include "editor/button_actions.h"
#include "vpin_wrapper.h"
#include "log/logging.h"
#include <imgui.h>
#include <imgui_internal.h>
#include <filesystem>
#include <cstdlib>
#include <cctype>
#include <chrono>
#include <algorithm>

namespace fs = std::filesystem;

ButtonActions::ButtonActions(IConfigService* config)
    : config_(config) {}

void ButtonActions::extractVBS(const std::string& filepath) {
    if (!config_) {
        LOG_ERROR("Config service is null, cannot extract VBS.");
        return;
    }
    const auto& settings = config_->getSettings();

    // Check if the user wants to use the external vpxtool
    if (settings.useVpxtool) {
        // --- External vpxtool Logic ---
        LOG_DEBUG("Using external 'vpxtool' for VBS extraction.");

        // Use vpxtoolBin setting, fallback to PATH
        std::string vpxtoolPath = "vpxtool";
        if (!settings.vpxtoolBin.empty() && fs::exists(settings.vpxtoolBin)) {
            vpxtoolPath = settings.vpxtoolBin;
        } else if (!settings.vpxtoolBin.empty()) {
            LOG_WARN("vpxtoolBin setting is specified but not found: " + settings.vpxtoolBin + ". Falling back to PATH.");
        }

        std::string cmd = "\"" + vpxtoolPath + "\" " + settings.vpxtoolExtractCmd + " \"" + filepath + "\"";
        LOG_DEBUG("Extracting VBS with command: " + cmd);
        int result = system(cmd.c_str());
        if (result != 0) {
            LOG_ERROR("Failed to extract VBS from table: " + filepath + " (command: " + cmd + ")");
        }
    } else {
        // --- Use VPinballX to extract Logic ---
        LOG_INFO("Using VPinballX for VBS extraction.");
        // Build command
        // std::string command = settings.VPinballXPath + " " + settings.vpxExtractCmd + " \"" + filepath;
        std::string command = "\"" + settings.VPinballXPath + "\" " + settings.vpxExtractCmd + " \"" + filepath + "\"";
        LOG_DEBUG("Command: " + command);
        int result = system(command.c_str());
        if (result != 0) {
            LOG_ERROR("VPinballX failed to extract VBS from table: " + filepath + " (command: " + command + ")");
        }
    }
}

bool ButtonActions::openInExternalEditor(const std::string& filepath) {
    std::string cmd = "xdg-open \"" + filepath + "\"";
    LOG_DEBUG("Attempting to open in external editor with: " + cmd);
    if (system(cmd.c_str()) != 0) {
        LOG_WARN("xdg-open failed. (You could add a 'fallbackEditor' to your Settings struct like in the old app).");
        return false;
    }
    return true;
}

void ButtonActions::openFolder(const std::string& filepath) {
    std::string folder;
    if (filepath.empty()) {
        folder = config_ ? config_->getSettings().VPXTablesPath : "."; // fallback to tables dir if possible
    } else {
        fs::path filePathObj(filepath);
        folder = filePathObj.parent_path().string();
    }

    if (folder.empty() || !fs::exists(folder)) {
        LOG_ERROR("Cannot open folder, invalid path: " + folder);
        return;
    }

    std::string cmd = "xdg-open \"" + folder + "\"";
    int result = system(cmd.c_str());
    if (result != 0) {
        LOG_ERROR("Failed to open folder: " + folder + " (exit code " + std::to_string(result) + ")");
    } else {
        LOG_DEBUG("Opened folder: " + folder);
    }
}

void ButtonActions::handleKeyboardSearchFocus(char* searchBuffer,
    std::string& searchQuery,
    std::function<void()> filterAndSort,
    std::function<void()> onEnter)
{
    ImGuiIO& io = ImGui::GetIO();

    // --- Global ENTER trigger (always works, no matter what is focused) ---
    if (ImGui::IsKeyPressed(ImGuiKey_Enter) || ImGui::IsKeyPressed(ImGuiKey_KeypadEnter)) {
        if (onEnter) onEnter();
        return;
    }

    if (io.WantCaptureKeyboard) return;
    if (ImGui::IsAnyItemActive()) return;

    static bool pendingFocus = false;

    // Cancel pending focus if ESC
    if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
    pendingFocus = false;

        // If user has text in the buffer, clear it no matter what
        if (searchBuffer[0] != '\0') {
            searchBuffer[0] = '\0';
            searchQuery.clear();
            if (filterAndSort) filterAndSort();
            // ImGui::ClearActiveID(); // defocus after clearing
            ImGuiContext* ctx = ImGui::GetCurrentContext();
            ctx->ActiveId = 0;
        } else {
            // If already empty, just unfocus input
            // ImGui::ClearActiveID();
            ImGuiContext* ctx = ImGui::GetCurrentContext();
            ctx->ActiveId = 0;
        }
        return;
    }

    // If we’re waiting to focus next frame
    if (pendingFocus) {
        ImGui::SetKeyboardFocusHere(-1); // Focus InputText on next draw
        pendingFocus = false;
        return;
    }

    // Detect first character typed
    for (int n = 0; n < io.InputQueueCharacters.Size; n++) {
        ImWchar c = io.InputQueueCharacters[n];
        if (std::isalnum(static_cast<unsigned int>(c)) || std::isspace(static_cast<unsigned int>(c))) {
            // Set focus to search bar next frame
            ImGui::SetKeyboardFocusHere(); // immediate focus this frame
            io.AddInputCharacter(c);       // forward first typed key into focused widget
            // io.InputQueueCharacters.clear();
            pendingFocus = false;

            // Reflect new buffer into string (for external filtering)
            searchQuery = searchBuffer;
            if (filterAndSort) filterAndSort();
            break;
        }
    }
}

void ButtonActions::launchTableWithStats(
    const TableData& selectedTable,
    std::vector<TableData>& masterTables,
    ITableLauncher* launcher,
    std::function<void()> refreshUICallback)
{
    if (!launcher) {
        LOG_ERROR("Launcher dependency is null, cannot launch table.");
        return;
    }

    const std::string& vpxFilePath = selectedTable.vpxFile;

    // 1. Find the mutable table object in the master list using the unique file path
    auto it = std::find_if(masterTables.begin(), masterTables.end(),
        [&vpxFilePath](const TableData& t) {
            return t.vpxFile == vpxFilePath;
        });

    if (it == masterTables.end()) {
        LOG_ERROR("Internal Error: Selected table not found in master tables list by path.");
        return;
    }

    TableData& t_mutable = *it;

    LOG_INFO("Editor: Launching table: " + t_mutable.title);

    // 2. LAUNCH THE TABLE AND TIME IT
    auto startTime = std::chrono::high_resolution_clock::now();

    // launchTable returns std::pair<int, float> (exit_code, time_played_in_seconds)
    auto [result, timePlayedSeconds] = launcher->launchTable(t_mutable);

    auto endTime = std::chrono::high_resolution_clock::now();

    // If the launcher didn't return time, calculate elapsed time
    if (timePlayedSeconds <= 0) {
        timePlayedSeconds = std::chrono::duration<float>(endTime - startTime).count();
    }

    float duration_minutes = timePlayedSeconds / 60.0f;

    // 3. UPDATE STATS
    if (result == 0) {
        // Success
        t_mutable.isBroken = false;
        t_mutable.playCount++;
        t_mutable.playTimeLast = duration_minutes;
        t_mutable.playTimeTotal += duration_minutes;
        LOG_INFO("Table launched successfully. Play time: " + std::to_string(duration_minutes) + " mins.");
    } else {
        // Failure
        t_mutable.isBroken = true;
        LOG_ERROR("Table launch failed with exit code " + std::to_string(result) + ". Marked as broken.");
    }

    // 4. SYNCHRONIZE UI
    if (refreshUICallback) {
        refreshUICallback();
    }
}
