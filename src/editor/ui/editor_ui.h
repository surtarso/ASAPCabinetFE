#pragma once

#include "editor/footer_actions.h"
#include "editor/sorting_filters.h"
#include "config/iconfig_service.h"
#include "tables/itable_loader.h"
#include "data/table_data.h"
#include "tables/itable_callbacks.h"
#include "launcher/itable_launcher.h"
#include "capture/iscreenshot_manager.h"
#include "core/ui/loading_progress.h"
#include "core/ui/modal_dialog.h"
#include <imgui.h>
#include <vector>
#include <string>
#include <memory>
#include <mutex>
#include <atomic>

enum class ScannerMode { File, VPin, VPSDb, HasIndex, Patch, MediaDb };

class TablePatcher;

/**
 * Orchestrator for the Editor UI.
 * Delegates rendering to header, body, footer & first-run components.
 */
class EditorUI {
public:
    EditorUI(bool& showMeta,
             bool& showView,
             bool& showBrowser,
             bool& showSettings,
             bool& showDownloadMediaPanel,
             bool& hotReload,
             IConfigService* config,
             ITableLoader* tableLoader,
             ITableLauncher* launcher,
             ITableCallbacks* tableCallbacks,
             std::shared_ptr<LoadingProgress> progress,
             IScreenshotManager* screenshotManager,
             TablePatcher* patcher
             );

    void draw();
    bool shouldExit() const { return exitRequested_; }

    // Fields and helpers for sub-modules:
    ButtonActions& actions()             { return actions_; }
    ITableLauncher* tableLauncher()     { return tableLauncher_; }
    IConfigService* configService()     { return config_; }
    TablePatcher* tablePatcher()     { return tablePatcher_; }

    std::vector<TableData>& tables()          { return tables_; }
    const std::vector<TableData>& filteredTables() const { return filteredTables_; }

    int&  selectedIndex()        { return selectedIndex_; }
    int  selectedIndex()        const { return selectedIndex_; }
    void setSelectedIndex(int i)       { selectedIndex_ = i; }

    bool loading()              const { return loading_; }
    void setLoading(bool l)            { loading_ = l; }

    std::string& searchQuery() { return searchQuery_; }
    void setSearchQuery(const std::string& q)   { searchQuery_ = q; }

    int  sortColumn()           const { return sortColumn_; }
    void setSortColumn(int c)           { sortColumn_ = c; }

    bool sortAscending()        const { return sortAscending_; }
    void setSortAscending(bool asc)       { sortAscending_ = asc; }

    ScannerMode scannerMode()   const { return selectedScanner_; }
    void setScannerMode(ScannerMode m)       { selectedScanner_ = m; }

    char* searchBuffer()                  { return searchBuffer_; }
    constexpr size_t searchBufferSize() const { return sizeof(searchBuffer_); }

    void filterAndSortTablesPublic()     { filterAndSortTables(); }
    void rescanAsyncPublic(ScannerMode mode) { rescanAsync(mode); }


    void setScrollToSelected(bool v) { scrollToSelected_ = v; }
    bool scrollToSelected() const { return scrollToSelected_; }

    void requestExit() { exitRequested_ = true; }

    std::mutex& tableMutex()            { return tableMutex_; }

    bool showMetadataEditor() const { return showMetadataEditor_; }
    void setShowMetadataEditor(bool v) { showMetadataEditor_ = v; }

    bool showMetadataView() const { return showMetadataView_; }
    void setShowMetadataView(bool v) { showMetadataView_ = v; }

    bool showVpsdbBrowser() const { return showVpsdbBrowser_; }
    void setShowVpsdbBrowser(bool v) { showVpsdbBrowser_ = v; }

    bool showEditorSettings() const { return showEditorSettings_; }
    void setShowEditorSettings(bool v) { showEditorSettings_ = v; }

    bool showDownloadMediaPanel() const { return showDownloadMediaPanel_; }
    void setShowDownloadMediaPanel(bool v) { showDownloadMediaPanel_ = v; }

    bool showHotReloadStatus() const { return hotReload_; }
    void setHotReloadStatus(bool v) { hotReload_ = v; }

    bool isConfigValid() const { return configValid_; }
    void setConfigValid(bool v) { configValid_ = v; }

    ModalDialog& modal() { return modal_; }
    std::function<void()> deferredModal_;

    void requestPostLaunchCleanup() { postLaunchCleanupRequired_ = true; }

    IScreenshotManager* screenshotManager() const { return screenshotManager_; }
    bool inExternalAppMode_ = false;              ///< Are we currently in an external app (like screenshot)?
    Uint32 lastExternalAppReturnTime_ = 0;        ///< Timestamp of last external app return
    bool screenshotModeActive_ = false;           ///< Is screenshot mode active
    static constexpr Uint32 EXTERNAL_APP_DEBOUNCE_TIME_MS = 500; ///< 0.5 sec debounce

private:
    // helper methods
    void filterAndSortTables();
    void rescanAsync(ScannerMode mode);

    // dependencies
    IConfigService* config_;
    ITableLoader* tableLoader_;
    ITableLauncher* tableLauncher_;
    ITableCallbacks* tableCallbacks_;
    std::shared_ptr<LoadingProgress> loadingProgress_;
    ButtonActions actions_;
    EditorTableFilter tableFilter_;

    // state
    ScannerMode selectedScanner_ = ScannerMode::File;

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

    bool configValid_ = false;

    bool& showMetadataEditor_;
    bool& showMetadataView_;
    bool& showVpsdbBrowser_;
    bool& showEditorSettings_;
    bool& showDownloadMediaPanel_;

    bool& hotReload_;

    ModalDialog modal_;
    IScreenshotManager* screenshotManager_;
    TablePatcher* tablePatcher_ = nullptr;

    std::atomic<bool> postLaunchCleanupRequired_ = false;
};
