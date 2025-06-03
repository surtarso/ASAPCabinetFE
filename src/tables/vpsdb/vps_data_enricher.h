#ifndef VPS_DATA_ENRICHER_H
#define VPS_DATA_ENRICHER_H

#include <json.hpp>
#include "tables/table_data.h"
#include "vps_utils.h"

class VpsDataEnricher {
public:
    VpsDataEnricher(const nlohmann::json& vpsDb);
    bool enrichTableData(const nlohmann::json& vpxTable, TableData& tableData) const;

private:
    const nlohmann::json& vpsDb_;
    VpsUtils utils_;
};

#endif // VPS_DATA_ENRICHER_H