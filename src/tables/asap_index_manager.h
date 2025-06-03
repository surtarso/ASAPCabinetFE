#ifndef ASAP_INDEX_MANAGER_H
#define ASAP_INDEX_MANAGER_H

#include "tables/table_data.h"
#include "config/settings.h"
#include <vector>

class AsapIndexManager {
public:
    static bool load(const Settings& settings, std::vector<TableData>& tables);
    static bool save(const Settings& settings, const std::vector<TableData>& tables);
};

#endif // ASAP_INDEX_MANAGER_H