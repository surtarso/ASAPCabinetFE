/**
 * @file vps_database_loader.h
 * @brief Defines the VpsDatabaseLoader class for loading the VPS database in ASAPCabinetFE.
 *
 * This header provides the VpsDatabaseLoader class, which loads the VPS database
 * (vpsdb.json) from a specified file path into a JSON object. The class supports
 * progress tracking via LoadingProgress and provides access to the loaded JSON data.
 * The loading process can be extended with configUI for custom validation rules in
 * the future.
 */

#ifndef VPS_DATABASE_LOADER_H
#define VPS_DATABASE_LOADER_H // Header guard to prevent multiple inclusions

#include <string> // For std::string to handle file paths
#include <nlohmann/json.hpp> // For nlohmann::json to store the parsed VPS database
#include "core/ui/loading_progress.h" // Structure for tracking loading progress

/**
 * @class VpsDatabaseLoader
 * @brief Loads the VPS database file in ASAPCabinetFE.
 *
 * This class handles the loading of the VPS database (vpsdb.json) from a specified
 * file path into a nlohmann::json object. It validates the JSON structure (array or
 * object with a 'tables' array), tracks progress with LoadingProgress, and provides
 * access to the loaded data. The path is configurable via the constructor, with
 * potential for configUI enhancements (e.g., custom validation rules).
 */
class VpsDatabaseLoader {
public:
    /**
     * @brief Constructs a VpsDatabaseLoader instance.
     *
     * Initializes the loader with the path to the VPS database file, which will be
     * used during the loading process.
     *
     * @param vpsDbPath The file path to the VPS database (vpsdb.json).
     */
    VpsDatabaseLoader(const std::string& vpsDbPath);

    /**
     * @brief Loads the VPS database from the specified file path.
     *
     * Parses the vpsdb.json file at vpsDbPath_ into a nlohmann::json object. Validates
     * that the JSON is either an array or an object with a 'tables' array, adjusting
     * the internal storage accordingly. Progress is tracked via LoadingProgress if
     * provided, updating currentTablesLoaded with the number of entries loaded.
     *
     * @param progress Optional pointer to LoadingProgress for real-time updates.
     * @return True if loading succeeds, false on failure (e.g., file not found, invalid JSON).
     */
    bool load(LoadingProgress* progress = nullptr);

    /**
     * @brief Retrieves the loaded VPS database JSON.
     *
     * Returns a constant reference to the loaded JSON data, which is either an array
     * of table entries or adjusted to be so from an object with a 'tables' array.
     *
     * @return A constant reference to the loaded VPS database JSON.
     */
    const nlohmann::json& getVpsDb() const;

private:
    std::string vpsDbPath_; ///< The file path to the VPS database (vpsdb.json).
    nlohmann::json vpsDb_; ///< The parsed JSON data from the VPS database.
};

#endif // VPS_DATABASE_LOADER_H