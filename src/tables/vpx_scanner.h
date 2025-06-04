#ifndef VPX_SCANNER_H
#define VPX_SCANNER_H

#include "tables/table_data.h"
#include "config/settings.h"
#include "core/loading_progress.h"
#include <vector>

class VpxScanner {
public:
    static std::vector<TableData> scan(const Settings& settings, LoadingProgress* progress = nullptr);
};

#endif // VPX_SCANNER_H