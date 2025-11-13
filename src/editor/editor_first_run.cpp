#include "editor/editor_first_run.h"
#include "config/settings.h"
#include "log/logging.h"
#include <filesystem>
#include <imgui.h>

namespace fs = std::filesystem;

namespace editor_first_run {

void drawFirstRun(EditorUI& ui) {
    if(!ui.isConfigValid()) {
        ImGui::Separator();
        ImGui::Text("Quick Setup: Check your paths, click Save and then Rescan to continue.");

        Settings& mutableSettings = ui.configService()->getMutableSettings();
        static char tablesPathBuf[1024];
        static char vpxPathBuf[1024];
        static bool initialized = false;

        // if (!initialized) {
        //     strncpy(tablesPathBuf, mutableSettings.VPXTablesPath.c_str(), sizeof(tablesPathBuf));
        //     strncpy(vpxPathBuf, mutableSettings.VPinballXPath.c_str(), sizeof(vpxPathBuf));
        //     initialized = true;
        // }

        if (!initialized) {
            // strncpy copy at most sizeof(tablesPathBuf) characters.
            // If the source string is >= 1024 characters, it won't be null-terminated.
            strncpy(tablesPathBuf, mutableSettings.VPXTablesPath.c_str(), sizeof(tablesPathBuf));

            // Manually ensure null termination
            tablesPathBuf[sizeof(tablesPathBuf) - 1] = '\0';

            strncpy(vpxPathBuf, mutableSettings.VPinballXPath.c_str(), sizeof(vpxPathBuf));

            // Manually ensure null termination
            vpxPathBuf[sizeof(vpxPathBuf) - 1] = '\0';

            initialized = true;
        }

        ImGui::InputText("Tables Folder", tablesPathBuf, sizeof(tablesPathBuf));
        ImGui::InputText("VPX Executable", vpxPathBuf, sizeof(vpxPathBuf));

        if (ImGui::Button("Save Paths##FirstRun")) {
            mutableSettings.VPXTablesPath = tablesPathBuf;
            mutableSettings.VPinballXPath = vpxPathBuf;
            ui.configService()->saveConfig();

            // Re-check validity once, update the cached state, and initiate scan if successful.
            bool isValid = ui.configService()->isConfigValid();
            ui.setConfigValid(isValid);

            LOG_INFO("First-run paths updated by user, validating...");

            if (isValid) {
                LOG_INFO("Paths are valid. Starting initial table load.");
                // If valid, trigger the table scan.
                ui.rescanAsyncPublic(ScannerMode::File);
            } else {
                LOG_WARN("Paths still invalid. Please check folder locations.");
            }
        }
    }
}
}
