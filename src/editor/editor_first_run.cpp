#include "editor/editor_first_run.h"
#include "config/settings.h"
#include "log/logging.h"
#include <filesystem>
#include <imgui.h>

namespace fs = std::filesystem;

namespace editor_first_run {

void drawFirstRun(EditorUI& ui) {
    // This logic executes only when ui.tables() is empty
    Settings settings = ui.configService()->getSettings();

    std::string tablesPath = settings.VPXTablesPath;
    if (!fs::exists(tablesPath)) {
        ImGui::TextColored(ImVec4(1,0.5f,0.5f,1),
                           "Tables path does not exist:\n%s", tablesPath.c_str());
        ImGui::TextDisabled("Please set a valid tables folder in settings.");
    } else {
        bool hasAnyFiles = false;
        for (auto& p : fs::recursive_directory_iterator(tablesPath)) {
            if (p.path().extension() == ".vpx") {
                hasAnyFiles = true; break;
            }
        }
        if (!hasAnyFiles) {
            ImGui::TextColored(ImVec4(1,0.8f,0.2f,1),
                               "No .vpx tables found in:\n%s", tablesPath.c_str());
            ImGui::TextDisabled("Please point to a folder containing .vpx tables.");
        } else {
            ImGui::TextDisabled("No table index found, first run?\nPlease pick a scanner and run a rescan tables.");
        }
    }

    std::string vpxPath = settings.VPinballXPath;
    if (!fs::exists(vpxPath)) {
        ImGui::TextColored(ImVec4(1,0.5f,0.5f,1),
                           "VPX executable not found:\n%s", vpxPath.c_str());
        ImGui::TextDisabled("Please set the correct path to VPinballX executable in settings.");
    } else if (!fs::is_regular_file(vpxPath)) {
        ImGui::TextColored(ImVec4(1,0.5f,0.5f,1),
                           "VPX path is not a file:\n%s", vpxPath.c_str());
        ImGui::TextDisabled("Please point to the actual VPinballX executable binary.");
    } else if ((fs::status(vpxPath).permissions() & fs::perms::owner_exec) == fs::perms::none) {
        ImGui::TextColored(ImVec4(1,0.8f,0.2f,1),
                           "VPX file is not executable:\n%s", vpxPath.c_str());
        ImGui::TextDisabled("Please make the file executable (chmod +x).");
    }

    ImGui::Separator();
    ImGui::Text("Quick Setup: Correct missing paths");

    Settings& mutableSettings = ui.configService()->getMutableSettings();
    static char tablesPathBuf[1024];
    static char vpxPathBuf[1024];
    static bool initialized = false;
    static bool pathsValid = false;

    if (!initialized) {
        strncpy(tablesPathBuf, mutableSettings.VPXTablesPath.c_str(), sizeof(tablesPathBuf));
        strncpy(vpxPathBuf, mutableSettings.VPinballXPath.c_str(), sizeof(vpxPathBuf));
        initialized = true;
    }

    ImGui::InputText("Tables Folder", tablesPathBuf, sizeof(tablesPathBuf));
    ImGui::InputText("VPX Executable", vpxPathBuf, sizeof(vpxPathBuf));

    if (ImGui::Button("Save Paths##FirstRun")) {
        mutableSettings.VPXTablesPath = tablesPathBuf;
        mutableSettings.VPinballXPath = vpxPathBuf;
        ui.configService()->saveConfig();
        LOG_INFO("First-run paths updated by user.");

        pathsValid = true;
        if (!fs::exists(tablesPathBuf) || !fs::is_directory(tablesPathBuf)) {
            pathsValid = false;
        } else {
            bool hasAny = false;
            for (auto& p : fs::recursive_directory_iterator(tablesPathBuf)) {
                if (p.path().extension() == ".vpx") {
                    hasAny = true; break;
                }
            }
            if (!hasAny) pathsValid = false;
        }
        if (!fs::exists(vpxPathBuf) ||
            !fs::is_regular_file(vpxPathBuf) ||
            ((fs::status(vpxPathBuf).permissions() & fs::perms::owner_exec)
             == fs::perms::none)) {
            pathsValid = false;
        }
    }

    if (pathsValid) {
        ImGui::TextColored(ImVec4(0.2f,0.8f,0.2f,1.0f),
                           "Paths saved and valid. Please pick a scanner and run a rescan.\n"
                           "If you already have an index, exit and re-open the editor.");
    }
}
}
