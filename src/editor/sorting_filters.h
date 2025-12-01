#pragma once

#include "data/table_data.h"
#include <vector>
#include <string>

/**
 * @brief Utility class to handle the complex filtering and sorting logic
 * for the EditorUI table list.
 *
 * This separates the data processing logic (search, sort, and selection rules)
 * from the UI rendering and state management.
 */
class EditorTableFilter {
public:
    /**
     * @brief Performs table filtering based on the search query and then sorts the results.
     *
     * @param sourceTables The complete list of tables (input).
     * @param targetFilteredTables The list to be populated with filtered and sorted results (output).
     * @param searchQuery The current string used for fuzzy searching.
     * @param sortColumn The column index used for sorting.
     * @param sortAscending The direction of the sort.
     * @param selectedIndex The index of the selected row, which may be updated (output).
     */
    void filterAndSort(const std::vector<TableData>& sourceTables,
                       std::vector<TableData>& targetFilteredTables,
                       const std::string& searchQuery,
                       int sortColumn,
                       bool sortAscending,
                       int& selectedIndex);
};
