#ifndef TABLE_LOADER_H
#define TABLE_LOADER_H

#include "tables/itable_loader.h"
#include "core/loading_progress.h"
#include <filesystem>

namespace fs = std::filesystem;

class TableLoader : public ITableLoader {
public:
    TableLoader() = default;
    std::vector<TableData> loadTableList(const Settings& settings, LoadingProgress* progress = nullptr) override;
    const std::map<char, int>& getLetterIndex() const override { return letterIndex; }

private:
    std::map<char, int> letterIndex;
    void sortTables(std::vector<TableData>& tables, const std::string& sortBy, LoadingProgress* progress);
};

#endif // TABLE_LOADER_H