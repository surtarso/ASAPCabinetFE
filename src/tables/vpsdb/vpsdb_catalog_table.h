#ifndef VPSDB_CATALOG_TABLE_H
#define VPSDB_CATALOG_TABLE_H

#include "vpsdb_metadata.h"
#include <string>
#include <queue>
#include <mutex>
#include <thread>

namespace vpsdb {

PinballTable loadTableFromJson(const std::string& vpsdbFilePath, size_t index);

// Struct to hold loaded table data (mirrors the original LoadedTableData)
struct LoadedTableData {
    size_t index;
    PinballTable table;
    std::string backglassPath;
    std::string playfieldPath;
};

// Loads table data in the background, including image downloads
void loadTableInBackground(const std::string& vpsdbFilePath, size_t index,
                           std::queue<LoadedTableData>& loadedTableQueue,
                           std::mutex& mutex, std::atomic<bool>& isTableLoading,
                        const std::string& exePath);

} // namespace vpsdb

#endif // VPSDB_CATALOG_TABLE_H