#ifndef VPSDB_CATALOG_LOADER_H
#define VPSDB_CATALOG_LOADER_H

#include "vpsdb_metadata.h"
#include "vps_database_client.h"
#include "config/settings.h"
#include "log/logging.h"
#include <string>
#include <vector>
#include <memory>
#include <atomic>
#include <thread>

namespace vpsdb {

class VpsdbJsonLoader {
public:
    VpsdbJsonLoader(const std::string& vpsdbFilePath, const Settings& settings);
    ~VpsdbJsonLoader();
    void initialize();
    bool isLoaded() const { return loaded_; }
    const std::vector<TableIndex>& getIndex() const { return index_; }
    int getProgressStage() const { return progressStage_; }
    bool isLoading() const { return isLoading_; }

private:
    std::string vpsdbFilePath_;
    const Settings& settings_;
    std::vector<TableIndex> index_;
    bool loaded_;
    std::atomic<bool> isLoading_;
    std::atomic<int> progressStage_; // 0: Not started, 1: Fetching, 2: Loading JSON, 3: Done
    std::unique_ptr<VpsDatabaseClient> vpsDbClient_;
    std::thread initThread_;

    void loadJson();
    void initInBackground();
};

} // namespace vpsdb

#endif // VPSDB_CATALOG_LOADER_H