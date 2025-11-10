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
                   bool& showSettings,
                   IConfigService* config,
                   ITableLoader* tableLoader,
                   ITableLauncher* launcher
                   )
    : showMetadataEditor_(showMeta),
      showVpsdbBrowser_(showBrowser),
      showEditorSettings_(showSettings),
      config_(config),
      tableLoader_(tableLoader),
      tableLauncher_(launcher),
      actions_(config)
{
    Settings settings = config_->getSettings();

    // maybe this should be in editor_body or first_run?
    if (config_->isConfigValid()) {
        LOG_INFO("Paths valid. Starting asynchronous load.");
        if (!settings.indexPath.empty()) {
            // Start with fast path (index load only)
            rescanAsync(ScannerMode::HasIndex);
        } else {
            // Index doesn't exist (first run on new machine/folder), perform a full file scan
            rescanAsync(ScannerMode::File);
        }

    } else {
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
            case ScannerMode::HasIndex:
                settings.ignoreScanners = true;
                settings.titleSource = "filename";
                settings.fetchVPSdb = false;
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
