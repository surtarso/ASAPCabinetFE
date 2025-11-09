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

    ui.configService()->isConfigValid(); // ensure we have the latest validity state

    ImGui::Separator();
    ImGui::Text("Quick Setup: Check your paths");

    Settings& mutableSettings = ui.configService()->getMutableSettings();
    static char tablesPathBuf[1024];
    static char vpxPathBuf[1024];
    static bool initialized = false;

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
        LOG_INFO("First-run paths updated by user, validating...");
        ui.configService()->isConfigValid();
    }
}
}
