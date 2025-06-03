#ifndef VPX_SCANNER_H
#define VPX_SCANNER_H

#include "tables/table_data.h"
#include "config/settings.h"
#include <vector>

class VpxScanner {
public:
    static std::vector<TableData> scan(const Settings& settings);
};

#endif // VPX_SCANNER_H