#pragma once
#include "editor/button_actions.h"
#include "tables/itable_loader.h"
#include "config/iconfig_service.h"
#include "tables/table_data.h"
#include "launcher/itable_launcher.h"
#include <imgui.h>
#include <vector>
#include <string>
#include <mutex>

/**
 * VPXGUITools port editor UI.
 * - Uses existing tableLoader_ and tables_ loaded from index.
 */
class EditorUI {
public:
    EditorUI(IConfigService* config, ITableLoader* tableLoader, ITableLauncher* launcher,
             bool& showMeta, bool& showIni);
    void draw();
    bool shouldExit() const { return exitRequested_; }

private:
    void rescanAsync();

    IConfigService* config_;       // Shared configuration interface
    ITableLoader* tableLoader_;    // Shared table loader
    ITableLauncher* tableLauncher_;
    ButtonActions actions_;

    std::vector<TableData> tables_;
    std::mutex tableMutex_;
    bool loading_ = false;
    bool exitRequested_ = false;

    // UI state (kept inside editor only)
    int selectedIndex_ = -1;
    bool scrollToSelected_ = false;

    int sortColumn_ = 1;     // Default sort column (1 = Name)
    bool sortAscending_ = true;

    // Booleans to control sub-editors (will be used later)
    bool& showMetadataEditor_;
    bool& showIniEditor_;
};
