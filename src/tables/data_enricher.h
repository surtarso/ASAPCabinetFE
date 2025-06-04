#ifndef DATA_ENRICHER_H
#define DATA_ENRICHER_H

#include "tables/table_data.h"
#include "config/settings.h"
#include "core/loading_progress.h"
#include <vector>
#include <json.hpp>

class DataEnricher {
public:
    static void enrich(const Settings& settings, std::vector<TableData>& tables, LoadingProgress* progress = nullptr);
    static std::string cleanString(const std::string& input);

private:
    static std::string safeGetString(const nlohmann::json& j, const std::string& key, const std::string& defaultValue = "");
};

#endif // DATA_ENRICHER_H