#ifndef VPXTOOL_SCANNER_H
#define VPXTOOL_SCANNER_H

#include "core/ui/loading_progress.h"
#include "config/settings.h"
#include "data/table_data.h"
#include <vector>
#include <string>

class VPXToolScanner {
public:
    static bool scanFiles(const Settings& settings, std::vector<TableData>& tables, LoadingProgress* progress);
};

#endif // VPXTOOL_SCANNER_H
