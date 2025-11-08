#include "editor/sorting_filters.h"
#include <algorithm>
#include <filesystem>
#include <functional>

namespace fs = std::filesystem;

namespace
{
    // Helper to compare two values with direction and a unique tie-breaker
    template <typename T>
    bool compareWithTieBreaker(const T &val_a, const T &val_b, bool ascending, const TableData &a, const TableData &b) {
        if (val_a == val_b) {
            // CRITICAL: Stable sort tie-breaker on unique file path
            return a.vpxFile < b.vpxFile;
        }
        return ascending ? (val_a < val_b) : (val_a > val_b);
    }

    // The actual sort logic wrapped in a function for clarity
    void performSort(std::vector<TableData> &tables, int sortColumn, bool sortAscending) {
        std::sort(tables.begin(), tables.end(), [sortColumn, sortAscending](const TableData &a, const TableData &b) {

            // Check if the current sort column is one of the complex text fields
            if (sortColumn == 1 || sortColumn == 3 || sortColumn == 4 || sortColumn == 6) {
                // Name (ID 1) should use 'title'
                if (sortColumn == 1) {
                    return compareWithTieBreaker(a.title, b.title, sortAscending, a, b);
                }
                // Author (ID 3) should use 'tableAuthor'
                if (sortColumn == 3) {
                    return compareWithTieBreaker(a.tableAuthor, b.tableAuthor, sortAscending, a, b);
                }
                // Manufacturer (ID 4) should use 'manufacturer'
                if (sortColumn == 4) {
                    return compareWithTieBreaker(a.manufacturer, b.manufacturer, sortAscending, a, b);
                }

                // ROM (ID 6) should use 'romName'
                if (sortColumn == 6) {
                    return compareWithTieBreaker(a.romName, b.romName, sortAscending, a, b);
                }
            }

            // Handle integer and simple string fields
            switch (sortColumn) {
                case 0: return compareWithTieBreaker(a.year, b.year, sortAscending, a, b);
                case 2: return compareWithTieBreaker(a.tableVersion, b.tableVersion, sortAscending, a, b);

                // If the sort column is not a dedicated data field (e.g., Files, Images, Videos),
                // default the primary sort to the Name (title).
                default: return compareWithTieBreaker(a.title, b.title, sortAscending, a, b);
            }
        });
    }
} // namespace

void EditorTableFilter::filterAndSort(const std::vector<TableData>& sourceTables,
                                      std::vector<TableData>& targetFilteredTables,
                                      const std::string& searchQuery,
                                      int sortColumn,
                                      bool sortAscending,
                                      int& selectedIndex)
{
    targetFilteredTables.clear();

    std::string lowerQuery;
    if (!searchQuery.empty()) {
        lowerQuery = searchQuery;
        // Use std::tolower from <cctype> via ::tolower
        std::transform(lowerQuery.begin(), lowerQuery.end(), lowerQuery.begin(), ::tolower);
    }

    for (const auto &table : sourceTables) {
        // --- FUZZY SEARCH LOGIC (from vpxguitools pattern) ---
        if (searchQuery.empty()) {
            targetFilteredTables.push_back(table);
            continue;
        }

        // Prepare fields for case-insensitive search
        std::string lowerTitle = table.title;
        std::transform(lowerTitle.begin(), lowerTitle.end(), lowerTitle.begin(), ::tolower);

        std::string lowerTableName = table.tableName;
        std::transform(lowerTableName.begin(), lowerTableName.end(), lowerTableName.begin(), ::tolower);

        std::string lowerVpsName = table.vpsName;
        std::transform(lowerVpsName.begin(), lowerVpsName.end(), lowerVpsName.begin(), ::tolower);


        fs::path vpxPath(table.vpxFile);
        std::string lowerFilename = vpxPath.stem().string();
        std::transform(lowerFilename.begin(), lowerFilename.end(), lowerFilename.begin(), ::tolower);

        std::string lowerRom = table.romName;
        std::transform(lowerRom.begin(), lowerRom.end(), lowerRom.begin(), ::tolower);

        // Check if query is found in Name (title), Filename (vpxFile stem), or ROM name
        if (lowerTitle.find(lowerQuery) != std::string::npos ||
            lowerTableName.find(lowerQuery) != std::string::npos ||
            lowerVpsName.find(lowerQuery) != std::string::npos ||
            lowerFilename.find(lowerQuery) != std::string::npos ||
            lowerRom.find(lowerQuery) != std::string::npos) {
            targetFilteredTables.push_back(table);
        }
    }

    // 1. Apply the existing sorting logic to the filtered list
    performSort(targetFilteredTables, sortColumn, sortAscending);

    // 2. Implement the selection rule:
    if (targetFilteredTables.empty()) {
        // If there are no tables (filtered list is empty), clear selection.
        selectedIndex = -1;
    } else if (searchQuery.empty()) {
        // If the search is CLEARED (full list is shown), explicitly unselect.
        selectedIndex = -1;
    } else {
        // If a search is active AND results were found, pre-select the first item.
        selectedIndex = 0;
    }
}
