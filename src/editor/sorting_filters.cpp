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

    // Case-insensitive, multi-author aware compare
    bool compareTextField(const std::string &aField, const std::string &bField, bool ascending,
                        const TableData &a, const TableData &b) {
        auto normalize = [](std::string s) {
            std::transform(s.begin(), s.end(), s.begin(), ::tolower);
            for (char c : {'+', '&', '/', ';'})
                std::replace(s.begin(), s.end(), c, ',');
            std::replace(s.begin(), s.end(), ' ', ',');
            return s;
        };

        std::string na = normalize(aField);
        std::string nb = normalize(bField);

        // Optional: split on ',' and compare first author for sorting stability
        std::stringstream sa(na), sb(nb);
        std::string fa, fb;
        std::getline(sa, fa, ',');
        std::getline(sb, fb, ',');

        return compareWithTieBreaker(fa, fb, ascending, a, b);
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
                // Author (ID 3) should use all table authors
                if (sortColumn == 3) {
                    return compareTextField(a.tableAuthor, b.tableAuthor, sortAscending, a, b);
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
        std::transform(lowerQuery.begin(), lowerQuery.end(), lowerQuery.begin(), ::tolower);
    }

    // Fuzzy search filtering
    for (const auto &table : sourceTables) {
        if (searchQuery.empty()) {
            targetFilteredTables.push_back(table);
            continue;
        }

        auto toLower = [](std::string s) {
            std::transform(s.begin(), s.end(), s.begin(), ::tolower);
            return s;
        };

        // ---- Normalize and combine equivalent metadata ----
        std::string titleCombo =
            toLower(table.title + " " + table.tableName + " " + table.vpsName);

        std::string authorCombo =
            toLower(table.tableAuthor + " " + table.vpsAuthors);

        std::string manufCombo =
            toLower(table.manufacturer + " " + table.tableManufacturer + " " + table.vpsManufacturer);

        std::string yearCombo =
            toLower(table.year + " " + table.tableYear + " " + table.vpsYear);

        fs::path vpxPath(table.vpxFile);
        std::string filename = toLower(vpxPath.stem().string());
        std::string rom = toLower(table.romName);

        // --- Split authors ---
        std::vector<std::string> authors;
        {
            std::string temp = authorCombo;
            size_t pos = 0;
            while ((pos = temp.find(" and ", pos)) != std::string::npos)
                temp.replace(pos, 5, ",");
            for (char c : {'+', '&', '/', ';'})
                std::replace(temp.begin(), temp.end(), c, ',');
            std::stringstream ss(temp);
            std::string token;
            while (std::getline(ss, token, ',')) {
                token.erase(0, token.find_first_not_of(" \t\r\n"));
                token.erase(token.find_last_not_of(" \t\r\n") + 1);
                if (!token.empty())
                    authors.push_back(token);
            }
        }

        // --- Match logic ---
        bool authorMatch = false;
        for (const auto& a : authors)
            if (a.find(lowerQuery) != std::string::npos) {
                authorMatch = true;
                break;
            }

        if (titleCombo.find(lowerQuery)     != std::string::npos ||
            manufCombo.find(lowerQuery)     != std::string::npos ||
            yearCombo.find(lowerQuery)      != std::string::npos ||
            filename.find(lowerQuery)       != std::string::npos ||
            rom.find(lowerQuery)            != std::string::npos ||
            authorMatch)
        {
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
