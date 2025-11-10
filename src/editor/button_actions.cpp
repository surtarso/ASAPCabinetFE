#include "editor/button_actions.h"
#include "vpin_wrapper.h"
#include "log/logging.h"
#include <imgui.h>
#include <imgui_internal.h>
#include <filesystem>
#include <cstdlib>
#include <cctype>

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
            io.InputQueueCharacters.clear();
            pendingFocus = false;

            // Reflect new buffer into string (for external filtering)
            searchQuery = searchBuffer;
            if (filterAndSort) filterAndSort();
            break;
        }
    }
}
