#include "tables/table_loader.h"
#include "tables/asap_index_manager.h"
#include "tables/vpx_scanner.h"
#include "tables/data_enricher.h"
#include "utils/logging.h"
#include <algorithm>

std::vector<TableData> TableLoader::loadTableList(const Settings& settings) {
    std::vector<TableData> tables;

    if (settings.titleSource == "metadata" && AsapIndexManager::load(settings, tables)) {
        LOG_INFO("TableLoader: Loaded from ASAP index");
    } else {
        tables = VpxScanner::scan(settings);
        if (settings.titleSource == "metadata") {
            DataEnricher::enrich(settings, tables);
            AsapIndexManager::save(settings, tables);
        }
    }

    std::sort(tables.begin(), tables.end(), [](const TableData& a, const TableData& b) {
        return a.title < b.title;
    });

    letterIndex.clear();
    for (size_t i = 0; i < tables.size(); ++i) {
        char firstChar = tables[i].title[0];
        if (std::isdigit(firstChar) || std::isalpha(firstChar)) {
            char key = std::isalpha(firstChar) ? std::toupper(firstChar) : firstChar;
            if (letterIndex.find(key) == letterIndex.end()) {
                letterIndex[key] = static_cast<int>(i);
            }
        }
    }

    return tables;
}