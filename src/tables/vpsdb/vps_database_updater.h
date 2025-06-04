#ifndef VPS_DATABASE_UPDATER_H
#define VPS_DATABASE_UPDATER_H

#include <string>
#include "core/loading_progress.h"

class VpsDatabaseUpdater {
public:
    VpsDatabaseUpdater(const std::string& vpsDbPath);
    bool fetchIfNeeded(const std::string& lastUpdatedPath, const std::string& updateFrequency, LoadingProgress* progress = nullptr);

private:
    bool downloadVpsDb(const std::string& url, LoadingProgress* progress);
    std::string vpsDbPath_;
};

#endif // VPS_DATABASE_UPDATER_H