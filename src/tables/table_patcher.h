#ifndef TABLE_PATCHER_H
#define TABLE_PATCHER_H

#include "tables/table_data.h"
#include "config/settings.h"
#include "core/loading_progress.h"
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

class TablePatcher {
public:
    /** Initiates the patching process for all tables based on settings and table data with progress tracking. */
    void patchTables(const Settings& settings, std::vector<TableData>& tables, LoadingProgress* progress = nullptr);

private:
    /** Downloads the hashes.json file from the specified GitHub URL. */
    std::string downloadHashesJson();

    /** Parses the downloaded JSON content into a nlohmann::json object. */
    nlohmann::json parseHashesJson(const std::string& jsonContent);

    /** Checks if a table needs a patch based on hash comparison. */
    bool needsPatch(const TableData& table, const nlohmann::json& hashes);

    /** Downloads a .vbs file from a URL and saves it to the specified path. */
    void downloadAndSaveVbs(const std::string& url, const std::string& savePath);
};

#endif // TABLE_PATCHER_H