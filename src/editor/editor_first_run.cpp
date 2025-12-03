#include "editor/editor_first_run.h"
#include "data/asapcab/asapcab_database_manager.h"
#include "config/settings.h"
#include "log/logging.h"
#include <filesystem>
#include <imgui.h>
#include <ImGuiFileDialog.h>

namespace fs = std::filesystem;

namespace editor_first_run {

void drawFirstRun(EditorUI& ui)
{
    if (!ui.isConfigValid())
    {
        ImGui::Separator();
        ImGui::Text("Quick Setup: Check your paths and click Save continue.");

        Settings& mutableSettings = ui.configService()->getMutableSettings();

        static char tablesPathBuf[1024];
        static char vpxPathBuf[1024];
        static bool initialized = false;

        if (!initialized)
        {
            strncpy(tablesPathBuf, mutableSettings.VPXTablesPath.c_str(), sizeof(tablesPathBuf));
            tablesPathBuf[sizeof(tablesPathBuf) - 1] = '\0';

            strncpy(vpxPathBuf, mutableSettings.VPinballXPath.c_str(), sizeof(vpxPathBuf));
            vpxPathBuf[sizeof(vpxPathBuf) - 1] = '\0';

            initialized = true;
        }

        // ====================================================================
        // TABLES FOLDER (copy of VPXTablesPath logic)
        // ====================================================================
        ImGui::Text("Tables Folder");
        ImGui::SameLine(140);
        ImGui::SetNextItemWidth(350);
        ImGui::InputText("##TablesFolderInput", tablesPathBuf, sizeof(tablesPathBuf));

        ImGui::SameLine();
        if (ImGui::Button("Browse##FirstRunTables"))
        {
            LOG_DEBUG("Browse button clicked for VPXTablesPath");

            IGFD::FileDialogConfig config;
            config.path = (!mutableSettings.VPXTablesPath.empty() && fs::exists(mutableSettings.VPXTablesPath))
                          ? fs::path(mutableSettings.VPXTablesPath).parent_path().string()
                          : std::string(getenv("HOME"));
            config.flags = ImGuiFileDialogFlags_Modal;

            auto* dlg = ImGuiFileDialog::Instance();
            dlg->SetFileStyle(IGFD_FileStyleByTypeDir, nullptr, ImVec4(0.5f, 1.0f, 0.9f, 0.9f));

            dlg->OpenDialog(
                "FolderDlg_VPXTablesPath_FirstRun",
                "Select VPX Tables Folder",
                nullptr,
                config
            );
        }

        // ====================================================================
        // VPX EXECUTABLE (copy of VPinballXPath logic)
        // ====================================================================
        ImGui::Text("VPX Executable");
        ImGui::SameLine(140);
        ImGui::SetNextItemWidth(350);
        ImGui::InputText("##VpxExecInput", vpxPathBuf, sizeof(vpxPathBuf));

        ImGui::SameLine();
        if (ImGui::Button("Browse##FirstRunVpxExec"))
        {
            LOG_DEBUG("Browse button clicked for VPinballXPath");

            IGFD::FileDialogConfig config;
            config.path = (!mutableSettings.VPinballXPath.empty() && fs::exists(mutableSettings.VPinballXPath))
                          ? fs::path(mutableSettings.VPinballXPath).parent_path().string()
                          : std::string(getenv("HOME"));
            config.flags = ImGuiFileDialogFlags_Modal;

            auto* dlg = ImGuiFileDialog::Instance();
            dlg->SetFileStyle(
                IGFD_FileStyleByFullName,
                "((VPinballX.*))",
                ImVec4(0.0f, 1.0f, 0.0f, 0.9f)
            );

            dlg->OpenDialog(
                "FileDlg_VPinballXPath_FirstRun",
                "Select VPinballX Executable",
                "((VPinballX.*))",
                config
            );
        }

        // ====================================================================
        // SAVE
        // ====================================================================
        if (ImGui::Button("Save Paths##FirstRun"))
        {
            mutableSettings.VPXTablesPath = tablesPathBuf;
            mutableSettings.VPinballXPath = vpxPathBuf;

            ui.configService()->saveConfig();

            bool isValid = ui.configService()->isConfigValid();
            ui.setConfigValid(isValid);

            if (isValid){
                ui.modal().openProgress(
                    "Initializing Database",
                    "Preparing AsapCab's Database...\nThis may take a few minutes."
                );

                std::thread([settings = ui.configService()->getSettings(), &ui]() {
                    data::asapcabdb::AsapCabDatabaseManager dbManager(settings);
                    bool success = dbManager.ensureAvailable();

                    if (!success) {
                        // Only show modal if DB build failed
                        ui.modal().openError(
                            "Failed to build AsapCab's Database",
                            "The database could not be built. Please check your internet connection.\n \
                            You will not be able to match tables to online metadata at this moment.\n \
                            Close this modal to start a complete file scan."
                        );
                        LOG_ERROR("Database failed on first run, starting complete file scan only.");
                        ui.rescanAsyncPublic(ScannerMode::VPin); // only scan files + metadata

                    } else {
                        // Success: directly start the scan without any pop-up
                        LOG_INFO("Database already present, starting full scan.");
                        ui.rescanAsyncPublic(ScannerMode::VPSDb); // scan (file + file metadata) + VPSDB (later main DB)
                    }

                }).detach();
            }
        }

        // ====================================================================
        // FILE DIALOG HANDLERS — EXACT COPY OF YOUR SETTINGS SYSTEM
        // ====================================================================

        auto* dlg = ImGuiFileDialog::Instance();
        ImVec2 minSize(500, 300);
        ImVec2 maxSize(900, 600);

        // VPX TABLES
        if (dlg->Display("FolderDlg_VPXTablesPath_FirstRun",
                         ImGuiWindowFlags_NoCollapse,
                         minSize, maxSize))
        {
            if (dlg->IsOk())
            {
                std::string selected = dlg->GetCurrentPath();
                strncpy(tablesPathBuf, selected.c_str(), sizeof(tablesPathBuf));
                tablesPathBuf[sizeof(tablesPathBuf) - 1] = '\0';

                LOG_INFO("Selected VPXTablesPath (First-Run): " + selected);
            }
            dlg->Close();
        }

        // VPINBALLX EXECUTABLE
        if (dlg->Display("FileDlg_VPinballXPath_FirstRun",
                         ImGuiWindowFlags_NoCollapse,
                         minSize, maxSize))
        {
            if (dlg->IsOk())
            {
                std::string selected = dlg->GetFilePathName();
                strncpy(vpxPathBuf, selected.c_str(), sizeof(vpxPathBuf));
                vpxPathBuf[sizeof(vpxPathBuf) - 1] = '\0';

                LOG_INFO("Selected VPinballXPath (First-Run): " + selected);
            }
            dlg->Close();
        }
    }
}

} // namespace editor_first_run
