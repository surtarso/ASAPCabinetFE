#ifndef ITABLE_LOADER_H
#define ITABLE_LOADER_H

#include <vector>
#include <map>
#include "render/table_data.h"
#include "config/settings.h"

class ITableLoader {
public:
    virtual ~ITableLoader() = default;
    virtual std::vector<TableData> loadTableList(const Settings& settings) = 0;
    virtual const std::map<char, int>& getLetterIndex() const = 0;
};

#endif // ITABLE_LOADER_H