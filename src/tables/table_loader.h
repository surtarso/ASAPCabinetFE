/**
 * @file table_loader.h
 * @brief Defines the TableLoader class for loading and managing table data in ASAPCabinetFE.
 *
 * This header provides the TableLoader class, which implements the ITableLoader interface
 * to load and process table data (e.g., VPX files) for ASAPCabinetFE. The class handles
 * a multi-stage process including fetching VPSDB, scanning files, enriching metadata,
 * saving indexes, and sorting tables. It supports progress tracking via LoadingProgress
 * and provides a letter-based index for navigation. The loading behavior is configurable
 * via Settings (e.g., titleSource, sortBy), with potential for further customization
 * via configUI in the future.
 */

#ifndef TABLE_LOADER_H
#define TABLE_LOADER_H // Header guard to prevent multiple inclusions

#include "tables/itable_loader.h" // Interface for table loading
#include "core/loading_progress.h" // Structure for tracking loading progress
#include <filesystem> // For std::filesystem namespace alias (fs) to handle file paths

namespace fs = std::filesystem; // Namespace alias for std::filesystem to simplify file path operations

/**
 * @class TableLoader
 * @brief Implements table loading and indexing for ASAPCabinetFE.
 *
 * This class loads a list of TableData by scanning VPX files, enriching metadata from
 * VPSDB or ASAP indexes, and sorting based on user-defined criteria (e.g., title, author).
 * It supports a five-stage process (fetching VPSDB, scanning, enriching, saving, sorting)
 * with optional progress tracking via LoadingProgress. The class maintains a letter index
 * for quick navigation and is configurable via Settings parameters, with extensibility
 * for configUI adjustments (e.g., sort options, metadata sources).
 */
class TableLoader : public ITableLoader {
public:
    /**
     * @brief Constructs a TableLoader instance.
     *
     * Default constructor initializes the loader with an empty letter index.
     */
    TableLoader() = default;

    /**
     * @brief Loads the list of table data based on application settings.
     *
     * Executes a multi-stage process to load tables: fetching VPSDB (if enabled),
     * scanning VPX files or loading from an ASAP index, enriching metadata, saving
     * the index, and sorting. Progress is tracked via LoadingProgress if provided.
     * The method is configurable via Settings (e.g., titleSource, fetchVPSdb, sortBy).
     *
     * @param settings The application settings controlling the loading process.
     * @param progress Optional pointer to LoadingProgress for real-time updates.
     * @return A vector of TableData representing the loaded tables.
     */
    std::vector<TableData> loadTableList(const Settings& settings, LoadingProgress* progress = nullptr) override;

    /**
     * @brief Retrieves the letter-based index for table navigation.
     *
     * Returns a map where keys are the first characters (uppercased letters or digits)
     * of table titles, and values are the corresponding indices in the sorted table list.
     *
     * @return A constant reference to the letter index map.
     */
    const std::map<char, int>& getLetterIndex() const override { return letterIndex; }

private:
    std::map<char, int> letterIndex; ///< Map of first characters (letters or digits) to indices for quick table navigation, built during sorting.
    /**
     * @brief Sorts the table list and rebuilds the letter index.
     *
     * Sorts the table data based on the specified criterion (e.g., title, author, year)
     * and updates the letterIndex map for navigation. Progress is tracked via
     * LoadingProgress if provided. The sort order is ascending except for "year"
     * (descending), and the index uses the first character of each title.
     *
     * @param tables The vector of TableData to sort.
     * @param sortBy The sorting criterion (e.g., "title", "author", "year").
     * @param progress Optional pointer to LoadingProgress for real-time updates.
     */
    void sortTables(std::vector<TableData>& tables, const std::string& sortBy, LoadingProgress* progress);
};

#endif // TABLE_LOADER_H