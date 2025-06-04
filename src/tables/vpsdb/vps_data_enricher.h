#ifndef VPS_DATA_ENRICHER_H
#define VPS_DATA_ENRICHER_H

#include <json.hpp>
#include "tables/table_data.h"
#include "vps_utils.h"
#include "core/loading_progress.h"

class VpsDataEnricher {
public:
    VpsDataEnricher(const nlohmann::json& vpsDb);
    bool enrichTableData(const nlohmann::json& vpxTable, TableData& tableData, LoadingProgress* progress = nullptr) const;

private:
    const nlohmann::json& vpsDb_;
    VpsUtils utils_;
    size_t levenshteinDistance(const std::string& s1, const std::string& s2) const;
};

#endif // VPS_DATA_ENRICHER_H