#ifndef VPS_DATABASE_CLIENT_H
#define VPS_DATABASE_CLIENT_H

#include <string>
#include "tables/table_data.h"
#include "vps_database_loader.h"
#include "vps_data_enricher.h"
#include "vps_database_updater.h"

class VpsDatabaseClient {
public:
    VpsDatabaseClient(const std::string& vpsDbPath);
    bool load();
    bool enrichTableData(const nlohmann::json& vpxTable, TableData& tableData) const;
    bool fetchIfNeeded(const std::string& lastUpdatedPath, const std::string& updateFrequency);

private:
    std::string vpsDbPath_;
    VpsDatabaseLoader loader_;
    VpsDataEnricher enricher_;
    VpsDatabaseUpdater updater_;
};

#endif // VPS_DATABASE_CLIENT_H