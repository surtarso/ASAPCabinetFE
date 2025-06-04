#ifndef DATA_ENRICHER_H
#define DATA_ENRICHER_H

#include "tables/table_data.h"
#include "config/settings.h"
#include <vector>
#include <json.hpp>

class DataEnricher {
public:
    static void enrich(const Settings& settings, std::vector<TableData>& tables);

private:
    static std::string cleanString(const std::string& input);
};

#endif // DATA_ENRICHER_H