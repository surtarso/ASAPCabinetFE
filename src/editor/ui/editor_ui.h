#pragma once

#include "editor/button_actions.h"
#include "editor/sorting_filters.h"
#include "tables/itable_loader.h"
#include "config/iconfig_service.h"
#include "tables/table_data.h"
#include "launcher/itable_launcher.h"
#include <imgui.h>
#include <vector>
#include <string>
#include <mutex>

enum class ScannerMode { File, VPin, VPSDb };

/**
 * Orchestrator for the Editor UI.
 * Delegates rendering to header, body, footer & first-run components.
 */
class EditorUI {
public:
    EditorUI(bool& showMeta,
             bool& showBrowser,
             IConfigService* config,
             ITableLoader* tableLoader,
             ITableLauncher* launcher
             );

    void draw();
    bool shouldExit() const { return exitRequested_; }

    // — Expose necessary fields and helpers for sub-modules:
    ButtonActions& actions()             { return actions_; }
    ITableLauncher* tableLauncher()     { return tableLauncher_; }
    IConfigService* configService()     { return config_; }

    const std::vector<TableData>& tables()         const { return tables_; }
    const std::vector<TableData>& filteredTables() const { return filteredTables_; }

    int  selectedIndex()        const { return selectedIndex_; }
    void setSelectedIndex(int i)       { selectedIndex_ = i; }

    bool loading()              const { return loading_; }
    void setLoading(bool l)            { loading_ = l; }

    // const std::string& searchQuery() const { return searchQuery_; }
    std::string& searchQuery() { return searchQuery_; }
    void setSearchQuery(const std::string& q)   { searchQuery_ = q; }

    int  sortColumn()           const { return sortColumn_; }
    void setSortColumn(int c)           { sortColumn_ = c; }

    bool sortAscending()        const { return sortAscending_; }
    void setSortAscending(bool asc)       { sortAscending_ = asc; }

    ScannerMode scannerMode()   const { return selectedScanner_; }
    void setScannerMode(ScannerMode m)       { selectedScanner_ = m; }

    bool forceRebuildMetadata() const { return forceRebuildMetadata_; }
    void setForceRebuildMetadata(bool v)     { forceRebuildMetadata_ = v; }

    bool useVpxtool()           const { return useVpxtool_; }
    void setUseVpxtool(bool v)               { useVpxtool_ = v; }

    char* searchBuffer()                  { return searchBuffer_; }

    void filterAndSortTablesPublic()     { filterAndSortTables(); }
    void rescanAsyncPublic(ScannerMode mode) { rescanAsync(mode); }

    void setScrollToSelected(bool v) { scrollToSelected_ = v; }
    bool scrollToSelected() const { return scrollToSelected_; }

    void requestExit() { exitRequested_ = true; }

    std::mutex& tableMutex()            { return tableMutex_; }

    bool& showMetadataEditor_;
    bool& showVpsdbBrowser_;

private:
    // helper methods
    void filterAndSortTables();
    void rescanAsync(ScannerMode mode);

    // dependencies
    IConfigService* config_;
    ITableLoader* tableLoader_;
    ITableLauncher* tableLauncher_;
    ButtonActions actions_;
    EditorTableFilter tableFilter_;

    // state
    ScannerMode selectedScanner_ = ScannerMode::File;
    bool forceRebuildMetadata_ = false;
    bool useVpxtool_            = false;

    std::vector<TableData> tables_;
    std::vector<TableData> filteredTables_;
    std::string searchQuery_;
    char searchBuffer_[256] = {0};

    std::mutex tableMutex_;
    bool loading_         = false;
    bool exitRequested_   = false;

    int  selectedIndex_      = -1;
    bool scrollToSelected_   = false;

    int  sortColumn_         = 1;
    bool sortAscending_      = true;
};
