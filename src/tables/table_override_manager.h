/**
 * @file table_override_manager.h
 * @brief Defines the TableOverrideManager class for handling per-table JSON overrides in ASAPCabinetFE.
 *
 * This header provides the TableOverrideManager class, which loads and applies overrides from
 * per-table JSON files (e.g., my_table.json) located in the same directory as the corresponding
 * VPX file (e.g., my_table.vpx). It supports merging user-specified TableData fields while
 * preserving the main asapcab_index.json. The class is designed for minimal integration with
 * existing components and includes placeholders for future dynamic reloading and UI-driven saving.
 */

#ifndef TABLE_OVERRIDE_MANAGER_H
#define TABLE_OVERRIDE_MANAGER_H

#include "tables/table_data.h"
#include <nlohmann/json.hpp>
#include <string>

class TableOverrideManager {
public:
    /**
     * @brief Constructs a TableOverrideManager instance.
     *
     * Initializes the manager without dependencies, relying on TableData::vpxFile for override paths.
     */
    TableOverrideManager() = default;

    /**
     * @brief Applies overrides from the table's JSON file to the TableData object.
     *
     * Loads the JSON file (<table_name>.json) from the same directory as table.vpxFile and merges
     * specified fields (e.g., title, playfieldVideo) into the TableData object. Only user-overrideable
     * fields are updated. Logs errors if the file is missing or invalid.
     *
     * @param table The TableData object to modify with override values.
     */
    void applyOverrides(TableData& table) const;

    /**
     * @brief Reloads overrides for a table (placeholder for dynamic reloading).
     *
     * Future implementation will reload overrides during runtime if the JSON file changes.
     *
     * @param table The TableData object to reload overrides for.
     */
    void reloadOverrides(TableData& table) const;

    /**
     * @brief Saves overrides to the table's JSON file.
     *
     * Writes user-edited fields to <table_name>.json, merging with existing fields to preserve
     * unedited ones. Logs errors if the file cannot be written.
     *
     * @param table The TableData object containing vpxFile for the file path.
     * @param overrides Map of field names to edited values.
     */
    void saveOverride(const TableData& table, const std::map<std::string, std::string>& overrides) const;
    
    /**
     * @brief Deletes the override JSON file for a table if it exists.
     *
     * Removes <table_name>.json from the same directory as table.vpxFile.
     *
     * @param table The TableData object containing vpxFile.
     */
    void deleteOverride(const TableData& table) const;

    /**
     * @brief Checks if the override JSON file exists for a table.
     *
     * @param table The TableData object containing vpxFile.
     * @return True if <table_name>.json exists, false otherwise.
     */
    bool overrideFileExists(const TableData& table) const;

private:
    /**
     * @brief Computes the path to the override JSON file.
     *
     * Derives <table_name>.json from table.vpxFile, located in the same directory.
     *
     * @param table The TableData object containing vpxFile.
     * @return The full path to the override JSON file, or empty string if invalid.
     */
    std::string getOverrideFilePath(const TableData& table) const;
};

#endif // TABLE_OVERRIDE_MANAGER_H