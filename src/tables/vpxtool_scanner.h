#pragma once

#include "tables/table_data.h"
#include "config/settings.h"
#include "core/loading_progress.h"
#include <vector>
#include <string>

class VPXToolScanner {
public:
    static bool scanFiles(const Settings& settings, std::vector<TableData>& tables, LoadingProgress* progress);
};