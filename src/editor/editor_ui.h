#pragma once
#include "editor/button_actions.h"
#include "tables/itable_loader.h"
#include "config/iconfig_service.h"
#include "tables/table_data.h"
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
    EditorUI(IConfigService* config, ITableLoader* tableLoader);
    void draw();
    bool shouldExit() const { return exitRequested_; }

private:
    void rescanAsync();

    IConfigService* config_;       // Shared configuration interface
    ITableLoader* tableLoader_;    // Shared table loader

    std::vector<TableData> tables_;
    std::mutex tableMutex_;
    bool loading_ = false;
    bool exitRequested_ = false;

    // UI state (kept inside editor only)
    int selectedIndex_ = -1;
    bool scrollToSelected_ = false;
    ButtonActions actions_;
};
