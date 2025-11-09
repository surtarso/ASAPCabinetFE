#include "editor/ui/editor_ui.h"
#include "editor/ui/editor_header.h"
#include "editor/ui/editor_body.h"
#include "editor/ui/editor_footer.h"
#include "editor/editor_first_run.h"
#include "config/settings.h"
#include "log/logging.h"
#include <filesystem>
#include <thread>

namespace fs = std::filesystem;

EditorUI::EditorUI(bool& showMeta,
                   bool& showBrowser,
                   IConfigService* config,
                   ITableLoader* tableLoader,
                   ITableLauncher* launcher
                   )
    : config_(config),
      tableLoader_(tableLoader),
      tableLauncher_(launcher),
      actions_(config),
      showMetadataEditor_(showMeta),
      showVpsdbBrowser_(showBrowser)
{
    Settings settings = config_->getSettings();
    settings.ignoreScanners = true;

    bool pathsValid = true;

    if (!fs::exists(settings.VPXTablesPath) ||
        !fs::is_directory(settings.VPXTablesPath)) {
        pathsValid = false;
    } else {
        bool hasVpx = false;
        for (auto& p : fs::recursive_directory_iterator(settings.VPXTablesPath)) {
            if (p.path().extension() == ".vpx") {
                hasVpx = true;
                break;
            }
        }
        if (!hasVpx) pathsValid = false;
    }

    if (!fs::exists(settings.VPinballXPath) ||
        !fs::is_regular_file(settings.VPinballXPath) ||
        ((fs::status(settings.VPinballXPath).permissions() & fs::perms::owner_exec)
         == fs::perms::none)) {
        pathsValid = false;
    }

    if (pathsValid) {
        tables_ = tableLoader_->loadTableList(settings, nullptr);
        filterAndSortTables();
    } else {
        tables_.clear();
        LOG_WARN("Critical paths invalid — skipping table load. User must correct paths first.");
    }
}

void EditorUI::draw() {
    ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoMove |
                                   ImGuiWindowFlags_NoResize |
                                   ImGuiWindowFlags_NoCollapse |
                                   ImGuiWindowFlags_NoTitleBar |
                                   ImGuiWindowFlags_NoSavedSettings;

    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);

    ImGui::Begin("ASAPCabinetFE Editor", nullptr, windowFlags);

    editor_header::drawHeader(*this);
    editor_body::drawBody(*this);
    editor_footer::drawFooter(*this);

    ImGui::End();
}

void EditorUI::filterAndSortTables() {
    tableFilter_.filterAndSort(tables_,
                               filteredTables_,
                               searchQuery_,
                               sortColumn_,
                               sortAscending_,
                               selectedIndex_);
}

void EditorUI::rescanAsync(ScannerMode mode) {
    if (loading_) return;
    loading_ = true;

    std::thread([this, mode]() {
        Settings settings = config_->getSettings();
        settings.ignoreScanners = false;
        settings.forceRebuildMetadata = forceRebuildMetadata_;
        settings.useVpxtool = useVpxtool_;

        switch (mode) {
            case ScannerMode::File:
                settings.titleSource = "filename";
                settings.fetchVPSdb = false;
                break;
            case ScannerMode::VPin:
                settings.titleSource = "metadata";
                settings.fetchVPSdb = false;
                break;
            case ScannerMode::VPSDb:
                settings.titleSource = "metadata";
                settings.fetchVPSdb = true;
                break;
        }

        auto newTables = tableLoader_->loadTableList(settings, nullptr);
        {
            std::lock_guard<std::mutex> lock(tableMutex_);
            tables_ = std::move(newTables);
            filterAndSortTables();
        }
        loading_ = false;
    }).detach();
}
