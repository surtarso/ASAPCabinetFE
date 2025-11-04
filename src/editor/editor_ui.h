#pragma once
#include "tables/itable_loader.h"
#include "config/iconfig_service.h"
#include <imgui.h>
#include <vector>
#include <string>
#include <mutex>

/**
 * @class EditorUI
 * @brief Minimal ImGui-based spreadsheet view for table management.
 *        Uses shared ASAPCabinetFE interfaces for table and config access.
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
};
