#include "editor/footer_actions.h"
#include "vpin_wrapper.h"
#include "log/logging.h"
#include <imgui.h>
#include <imgui_internal.h>
#include <filesystem>
#include <cstdlib>
#include <cctype>
#include <chrono>
#include <algorithm>
#include <thread>
#include <array>
#include <memory>

namespace fs = std::filesystem;

ButtonActions::ButtonActions(IConfigService* config, ITableCallbacks* tableCallbacks)
    : config_(config), tableCallbacks_(tableCallbacks) {}

void ButtonActions::extractOrOpenVbs(
    const std::string& filepath,
    std::function<void(const std::string&)> onOutput,
    std::function<void()> onFinished
) {
    fs::path vpxPath(filepath);
    fs::path vbsPath = vpxPath.parent_path() / (vpxPath.stem().string() + ".vbs");

    // ----------------------------------------------------------
    // 1) If VBS already exists → open it
    // ----------------------------------------------------------
    if (fs::exists(vbsPath)) {
        if (onOutput) {
            onOutput("VBS already exists:");
            onOutput(vbsPath.string());
            onOutput("Opening...");
        }

        openInExternalEditor(vbsPath.string());

        if (onFinished) onFinished();
        return;
    }

    // ----------------------------------------------------------
    // 2) VBS missing → extract it
    // ----------------------------------------------------------
    if (onOutput) {
        onOutput("VBS missing:");
        onOutput(vbsPath.string());
        onOutput("Extracting VBS from VPX...");
    }

    if (!config_) {
        if (onOutput) onOutput("ERROR: Config service is null.");
        if (onFinished) onFinished();
        return;
    }

    const auto& settings = config_->getSettings();

    // pick tool
    std::string toolPath;
    std::string toolCmd;

    if (settings.useVpxtool) {
        toolPath = settings.vpxtoolBin.empty() ? "vpxtool" : settings.vpxtoolBin;
        toolCmd  = settings.vpxtoolExtractCmd;
    } else {
        toolPath = settings.VPinballXPath;
        toolCmd  = settings.vpxExtractCmd;
    }

    // build command
    std::string cmd = "\"" + toolPath + "\" " + toolCmd + " \"" + filepath + "\"";

    if (onOutput) onOutput("Executing: " + cmd);

    // ----------------------------------------------------------
    // 3) Run async extraction
    // ----------------------------------------------------------
    std::thread([this, cmd, vbsPath, onOutput, onFinished]() {

        std::array<char, 256> buffer;
        std::unique_ptr<FILE, int(*)(FILE*)> pipe(popen(cmd.c_str(), "r"), pclose);

        if (!pipe) {
            if (onOutput) onOutput("ERROR: Failed to execute command.");
            if (onFinished) onFinished();
            return;
        }

        while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
            if (onOutput) onOutput(std::string(buffer.data()));
        }

        // ------------------------------------------------------
        // 4) After extraction → open it
        // ------------------------------------------------------
        if (fs::exists(vbsPath)) {
            if (onOutput) onOutput("Extraction complete. Opening VBS...");
            openInExternalEditor(vbsPath.string());
        } else {
            if (onOutput) {
                onOutput("ERROR: VBS not found after extraction:");
                onOutput(vbsPath.string());
                onOutput("Extraction failed.");
            }
        }

        if (onFinished) onFinished();

    }).detach();
}



// ----------------------------------------------------------
// External editor opener (unchanged)
// ----------------------------------------------------------
bool ButtonActions::openInExternalEditor(const std::string& filepath) {
    std::string cmd = "xdg-open \"" + filepath + "\"";
    LOG_DEBUG("Attempting to open in external editor with: " + cmd);
    if (system(cmd.c_str()) != 0) {
        LOG_WARN("xdg-open failed. (You could add a fallbackEditor to Settings).");
        return false;
    }
    return true;
}


void ButtonActions::openFolder(const std::string& filepath) {
    std::string folder;
    if (filepath.empty()) {
        folder = config_ ? config_->getSettings().VPXTablesPath : "."; // fallback
    } else {
        fs::path filePathObj(filepath);
        folder = filePathObj.parent_path().string();
    }

    if (folder.empty() || !fs::exists(folder)) {
        LOG_ERROR("Cannot open folder, invalid path: " + folder);
        return;
    }

    // Launch in a detached thread
    std::thread([folder]() {
        std::string cmd = "xdg-open \"" + folder + "\" >/dev/null 2>&1";
        int result = std::system(cmd.c_str());
        if (result != 0) {
            LOG_ERROR("Failed to open folder: " + folder + " (exit code " + std::to_string(result) + ")");
        } else {
            LOG_DEBUG("Opened folder: " + folder);
        }
    }).detach();
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
            ImGuiContext* ctx = ImGui::GetCurrentContext();
            ctx->ActiveId = 0;
        } else {
            // If already empty, just unfocus input
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

    // 1. Find table in master list
    auto it = std::find_if(masterTables.begin(), masterTables.end(),
        [&vpxFilePath](const TableData& t) {
            return t.vpxFile == vpxFilePath;
        });

    if (it == masterTables.end()) {
        LOG_ERROR("Internal Error: Selected table not found in master tables list by path.");
        return;
    }

    TableData& t_mutable = *it;

    LOG_INFO("Editor: Launching table: " + t_mutable.bestTitle);

    auto startTime = std::chrono::high_resolution_clock::now();

    // --- ASYNC LAUNCH ---
    launcher->launchTableAsync(
        t_mutable,
        [this, &t_mutable, &masterTables, refreshUICallback, startTime]
        (int result, float timePlayedSeconds)
        {
            auto endTime = std::chrono::high_resolution_clock::now();

            if (timePlayedSeconds <= 0) {
                timePlayedSeconds =
                    std::chrono::duration<float>(endTime - startTime).count();
            }

            float duration_minutes = timePlayedSeconds / 60.0f;

            // 3. UPDATE STATS
            if (result == 0) {
                t_mutable.isBroken = false;
                t_mutable.playCount++;
                t_mutable.playTimeLast = duration_minutes;
                t_mutable.playTimeTotal += duration_minutes;
                LOG_INFO("Table launched successfully. Play time: " +
                         std::to_string(duration_minutes) + " mins.");
            } else {
                t_mutable.isBroken = true;
                LOG_ERROR("Table launch failed with exit code " +
                          std::to_string(result) +
                          ". Marked as broken.");
            }

            // SAVE (unchanged)
            if (tableCallbacks_ && config_) {
                const auto& settings = config_->getSettings();
                if (tableCallbacks_->save(settings, masterTables, nullptr)) {
                    LOG_DEBUG("Table data updated and saved successfully via callback.");
                } else {
                    LOG_ERROR("Failed to save updated table data via callback.");
                }
            } else {
                LOG_ERROR("Cannot save table data: Missing TableCallbacks or ConfigService dependency.");
            }

            // REFRESH UI
            if (refreshUICallback) {
                refreshUICallback();
            }
        }
    );
}
