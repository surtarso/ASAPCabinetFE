#ifndef VPXTOOL_SCANNER_H
#define VPXTOOL_SCANNER_H

#include "table_data.h"
#include "config/settings.h"
#include "core/ui/loading_progress.h"
#include <vector>
#include <string>

class VPXToolScanner {
public:
    static bool scanFiles(const Settings& settings, std::vector<TableData>& tables, LoadingProgress* progress);
};

#endif // VPXTOOL_SCANNER_H