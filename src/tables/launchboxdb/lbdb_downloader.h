// src/tables/launchboxdb/lbdb_downloader.h
#pragma once
#include "config/settings.h"
#include "tables/table_data.h"
#include "core/ui/loading_progress.h"
#include <string>
#include <optional>

class LbdbDownloader {
public:
    LbdbDownloader(const Settings& s, LoadingProgress* p = nullptr)
        : settings_(s), progress_(p) {}

    void downloadArtForTables(std::vector<TableData>& tables);

private:
    const Settings& settings_;
    LoadingProgress* progress_;

    std::optional<std::string> findBestMatch(const TableData& table);
    // void downloadClearLogo(const std::string& gameId, TableData& table);
    void downloadClearLogo(const std::string& gameId,
                       TableData& table,
                       const nlohmann::json& db);
    void downloadFlyersFromJson(const std::string& gameId, TableData& table, const nlohmann::json& db);
};
