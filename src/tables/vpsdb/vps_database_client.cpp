#include "vps_database_client.h"
#include "utils/logging.h"

VpsDatabaseClient::VpsDatabaseClient(const std::string& vpsDbPath)
    : vpsDbPath_(vpsDbPath),
      loader_(vpsDbPath),
      enricher_(loader_.getVpsDb()),
      updater_(vpsDbPath) {}

bool VpsDatabaseClient::load() {
    return loader_.load();
}

bool VpsDatabaseClient::enrichTableData(const nlohmann::json& vpxTable, TableData& tableData) const {
    return enricher_.enrichTableData(vpxTable, tableData);
}

bool VpsDatabaseClient::fetchIfNeeded(const std::string& lastUpdatedPath, const std::string& updateFrequency) {
    return updater_.fetchIfNeeded(lastUpdatedPath, updateFrequency);
}