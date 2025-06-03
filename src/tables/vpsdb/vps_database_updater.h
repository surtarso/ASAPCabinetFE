#ifndef VPS_DATABASE_UPDATER_H
#define VPS_DATABASE_UPDATER_H

#include <string>

class VpsDatabaseUpdater {
public:
    VpsDatabaseUpdater(const std::string& vpsDbPath);
    bool fetchIfNeeded(const std::string& lastUpdatedPath, const std::string& updateFrequency);

private:
    bool downloadVpsDb(const std::string& url);
    std::string vpsDbPath_;
};

#endif // VPS_DATABASE_UPDATER_H