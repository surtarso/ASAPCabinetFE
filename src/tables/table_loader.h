#ifndef TABLE_LOADER_H
#define TABLE_LOADER_H

#include "tables/itable_loader.h"
#include <filesystem>

namespace fs = std::filesystem;

class TableLoader : public ITableLoader {
public:
    TableLoader() = default;
    std::vector<TableData> loadTableList(const Settings& settings) override;
    const std::map<char, int>& getLetterIndex() const override { return letterIndex; }

private:
    std::map<char, int> letterIndex;
};

#endif // TABLE_LOADER_H