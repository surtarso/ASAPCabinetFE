#include "vps_database_client.h"
#include "utils/logging.h"

VpsDatabaseClient::VpsDatabaseClient(const std::string& vpsDbPath)
    : vpsDbPath_(vpsDbPath),
      loader_(vpsDbPath),
      enricher_(loader_.getVpsDb()),
      updater_(vpsDbPath) {}

bool VpsDatabaseClient::load(LoadingProgress* progress) {
    return loader_.load(progress);
}

bool VpsDatabaseClient::enrichTableData(const nlohmann::json& vpxTable, TableData& tableData, LoadingProgress* progress) const {
    return enricher_.enrichTableData(vpxTable, tableData, progress);
}

bool VpsDatabaseClient::fetchIfNeeded(const std::string& lastUpdatedPath, const std::string& updateFrequency, LoadingProgress* progress) {
    return updater_.fetchIfNeeded(lastUpdatedPath, updateFrequency, progress);
}