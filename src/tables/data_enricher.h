#ifndef DATA_ENRICHER_H
#define DATA_ENRICHER_H

#include "tables/table_data.h"
#include "config/settings.h"
#include <vector>

class DataEnricher {
public:
    static void enrich(const Settings& settings, std::vector<TableData>& tables);
};

#endif // DATA_ENRICHER_H