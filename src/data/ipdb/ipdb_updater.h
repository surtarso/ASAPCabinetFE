#ifndef DATA_IPDB_IPDB_UPDATER_H
#define DATA_IPDB_IPDB_UPDATER_H

#include <string>
#include <filesystem>
#include <optional>
#include "config/settings.h"
#include "core/ui/loading_progress.h"

namespace data::ipdb {

class IpdbUpdater {
public:
    IpdbUpdater(const Settings& settings,
            LoadingProgress* progress = nullptr);

    // Ensures DB exists in cache. Downloads if missing.
    bool ensureAvailable();

    // Force a fresh download (UI button)
    bool forceUpdate();

private:
    bool download();

private:
    const Settings& settings_;
    LoadingProgress* progress_;
};

} // namespace data::ipdb

#endif
