#include "tables/vpin_scanner.h"
#include "tables/asap_index_manager.h" // Keep if used for saving, otherwise remove
#include "tables/vpsdb/vps_database_client.h"
#include "utils/logging.h"
#include "vpin_wrapper.h" // For get_vpx_table_info_as_json and free_rust_string
#include "path_utils.h"    // For PathUtils::cleanString, PathUtils::safeGetString
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>
#include <thread>
#include <future>
#include <vector>
#include <atomic>

namespace fs = std::filesystem;
using json = nlohmann::json;

void VPinScanner::scanFiles(const Settings& settings, std::vector<TableData>& tables, LoadingProgress* progress) {
    LOG_DEBUG("VPinFileScanner: Starting scan with vpin for " << tables.size() << " tables.");

    // Phase 1: VPin direct file scanning
    if (progress) {
        std::lock_guard<std::mutex> lock(progress->mutex);
        progress->currentTask = "Scanning VPX files with vpin...";
        progress->totalTablesToLoad = tables.size();
        progress->currentTablesLoaded = 0;
        progress->numMatched = 0;
        progress->numNoMatch = 0;
        progress->logMessages.push_back("DEBUG: Initialized vpin scan for " + std::to_string(tables.size()) + " tables");
    }

    // Load VPSDB if enabled (Load it once here, before any parallel processing)
    VpsDatabaseClient vpsClient(settings.vpsDbPath);
    bool vpsLoaded = false;
    if (settings.fetchVPSdb) {
        if (progress) {
            std::lock_guard<std::mutex> lock(progress->mutex);
            progress->currentTask = "Fetching/Loading VPSDB (VPin scan phase)..."; // Update task for VPSDB initial load
        }
        if (vpsClient.fetchIfNeeded(settings.vpsDbLastUpdated, settings.vpsDbUpdateFrequency, progress) && vpsClient.load(progress)) {
            vpsLoaded = true;
            LOG_INFO("VPinFileScanner: VPSDB loaded successfully for enrichment.");
        } else {
            LOG_ERROR("VPinFileScanner: Failed to load vpsdb.json for enrichment, proceeding without it.");
            // Even if failed, reset progress for next sub-phase
            if (progress) {
                std::lock_guard<std::mutex> lock(progress->mutex);
                progress->currentTask = "VPX file scanning...";
            }
        }
    } else {
        LOG_INFO("VPinFileScanner: VPSDB fetch disabled.");
    }

    std::vector<std::future<void>> futures;
    std::atomic<int> processedVpin(0); // Counter for VPin processed tables
    const size_t maxThreads = std::max(1u, std::thread::hardware_concurrency() / 2);

    for (auto& table : tables) {
        while (futures.size() >= maxThreads) {
            for (auto it = futures.begin(); it != futures.end();) {
                if (it->wait_for(std::chrono::milliseconds(0)) == std::future_status::ready) {
                    try {
                        it->get();
                    } catch (const std::exception& e) {
                        LOG_ERROR("VPinFileScanner: Thread exception during VPin scan for " << table.vpxFile << ": " << e.what());
                    }
                    it = futures.erase(it);
                } else {
                    ++it;
                }
            }
            std::this_thread::yield();
        }

        futures.push_back(std::async(std::launch::async, [&table, progress, &processedVpin]() {
            std::string vpxFile = table.vpxFile;
            LOG_DEBUG("VPinFileScanner: Processing VPX file with vpin: " << vpxFile);
            char* json_result = get_vpx_table_info_as_json(vpxFile.c_str());
            if (!json_result) {
                LOG_ERROR("VPinFileScanner: Failed to get metadata for " << vpxFile);
                if (progress) {
                    std::lock_guard<std::mutex> lock(progress->mutex);
                    progress->numNoMatch++; // This counter tracks successful VPin metadata extraction
                    progress->logMessages.push_back("DEBUG: Failed to process: " + vpxFile);
                }
                // Free the possibly null json_result to avoid issues
                free_rust_string(json_result);
                // Update processed count for this phase
                if (progress) {
                    std::lock_guard<std::mutex> lock(progress->mutex);
                    progress->currentTablesLoaded = ++processedVpin;
                    progress->currentTask = "VPX file scanning: " + std::to_string(processedVpin) + " of " + std::to_string(progress->totalTablesToLoad) + " files";
                }
                return;
            }

            try {
                std::string json_str(json_result);
                json vpinJson = json::parse(json_str);

                table.tableName = PathUtils::cleanString(PathUtils::safeGetString(vpinJson, "table_name", table.title));
                table.authorName = PathUtils::cleanString(PathUtils::safeGetString(vpinJson, "author_name", ""));
                table.tableDescription = PathUtils::cleanString(PathUtils::safeGetString(vpinJson, "table_description", ""));
                table.tableSaveDate = PathUtils::safeGetString(vpinJson, "table_save_date", "");
                table.releaseDate = PathUtils::safeGetString(vpinJson, "release_date", "");
                table.tableVersion = PathUtils::cleanString(PathUtils::safeGetString(vpinJson, "table_version", ""));
                table.tableRevision = PathUtils::safeGetString(vpinJson, "table_save_rev", "");
                table.title = table.tableName.empty() ? PathUtils::cleanString(fs::path(vpxFile).stem().string()) : table.tableName;
                table.jsonOwner = "VPin Filescan";

                if (vpinJson.contains("properties") && vpinJson["properties"].is_object()) {
                    table.manufacturer = PathUtils::cleanString(PathUtils::safeGetString(vpinJson["properties"], "manufacturer", ""));
                    table.year = PathUtils::cleanString(PathUtils::safeGetString(vpinJson["properties"], "year", ""));
                }
                // LOG_DEBUG("VPinFileScanner: vpin table scanned: path=" << table.vpxFile << ", title=" << table.title << ", tableName=" << table.tableName << ", manufacturer=" << table.manufacturer << ", year=" << table.year);

                if (progress) {
                    std::lock_guard<std::mutex> lock(progress->mutex);
                    progress->numMatched++; // This counter tracks successful VPin metadata extraction
                    progress->logMessages.push_back("DEBUG: Processed: " + vpxFile);
                }
            } catch (const json::exception& e) {
                LOG_ERROR("VPinFileScanner: JSON parsing error for " << vpxFile << ": " << e.what());
                if (progress) {
                    std::lock_guard<std::mutex> lock(progress->mutex);
                    progress->numNoMatch++; // This counter tracks successful VPin metadata extraction
                    progress->logMessages.push_back("DEBUG: JSON error: " + vpxFile);
                }
            } catch (...) {
                LOG_ERROR("VPinFileScanner: Unexpected error processing " << vpxFile);
                if (progress) {
                    std::lock_guard<std::mutex> lock(progress->mutex);
                    progress->numNoMatch++; // This counter tracks successful VPin metadata extraction
                    progress->logMessages.push_back("DEBUG: Unexpected error: " + vpxFile);
                }
            }

            free_rust_string(json_result);
            if (progress) {
                std::lock_guard<std::mutex> lock(progress->mutex);
                progress->currentTablesLoaded = ++processedVpin;
                progress->currentTask = "VPX file scanning: " + std::to_string(processedVpin) + " of " + std::to_string(progress->totalTablesToLoad) + " files";
            }
        }));
    }

    // Wait for all vpin threads to complete
    for (auto& future : futures) {
        try {
            future.get();
        } catch (const std::exception& e) {
            LOG_ERROR("VPinFileScanner: Thread exception: " << e.what());
        }
    }

    // Phase 2: VPSDB enrichment (after initial VPin scan is complete for all tables)
    if (vpsLoaded) {
        // Reset progress counters for the *new* VPSDB matching phase
        if (progress) {
            std::lock_guard<std::mutex> lock(progress->mutex);
            progress->currentTask = "Matching tables to VPSDB...";
            progress->currentTablesLoaded = 0; // Reset for this new sub-phase
            progress->totalTablesToLoad = tables.size(); // Still the same total tables
            progress->numMatched = 0;    // Reset VPSDB match count
            progress->numNoMatch = 0;    // Reset VPSDB no-match count
            progress->logMessages.push_back("DEBUG: Starting VPSDB enrichment for " + std::to_string(tables.size()) + " tables");
        }

        const size_t maxThreadsVps = std::max(1u, std::thread::hardware_concurrency() / 2);
        std::vector<std::future<void>> vpsFutures;
        std::atomic<int> processedVps(0);

        for (auto& table : tables) {
            while (vpsFutures.size() >= maxThreadsVps) {
                for (auto it = vpsFutures.begin(); it != vpsFutures.end();) {
                    if (it->wait_for(std::chrono::milliseconds(0)) == std::future_status::ready) {
                        try {
                            it->get();
                        } catch (const std::exception& e) {
                            LOG_ERROR("VPinFileScanner: VPSDB thread exception for " << table.vpxFile << ": " << e.what());
                        }
                        it = vpsFutures.erase(it);
                    } else {
                        ++it;
                    }
                }
                std::this_thread::yield();
            }

            vpsFutures.push_back(std::async(std::launch::async, [&table, &vpsClient, progress, &processedVps, &tables]() {
                // Construct a temporary JSON object for VPSDB matching, using data already extracted by VPin
                json tempVpinJsonForVps;
                tempVpinJsonForVps["path"] = table.vpxFile;
                tempVpinJsonForVps["game_name"] = table.romName; // Use romName from vpin scan if available
                tempVpinJsonForVps["table_info"] = {
                    {"table_name", table.tableName},
                    {"author_name", table.authorName},
                    {"table_description", table.tableDescription},
                    {"table_version", table.tableVersion},
                    {"table_save_date", table.tableSaveDate},
                    {"release_date", table.releaseDate},
                    {"table_save_rev", table.tableRevision}
                };
                tempVpinJsonForVps["properties"] = {
                    {"manufacturer", table.manufacturer},
                    {"year", table.year}
                };

                // LOG_DEBUG("VPinFileScanner: vpin table before VPSDB (enrichment phase): path=" << table.vpxFile << ", title=" << table.title << ", tableName=" << table.tableName << ", manufacturer=" << table.manufacturer << ", year=" << table.year);

                vpsClient.matchMetadata(tempVpinJsonForVps, table, progress);

                // LOG_DEBUG("VPinFileScanner: vpin table after VPSDB (enrichment phase): path=" << table.vpxFile << ", title=" << table.title << ", tableName=" << table.tableName << ", manufacturer=" << table.manufacturer << ", year=" << table.year);

                if (progress) {
                    std::lock_guard<std::mutex> lock(progress->mutex);
                    progress->currentTablesLoaded = ++processedVps;
                    progress->currentTask = "Matched " + std::to_string(processedVps) + " of " + std::to_string(tables.size()) + " tables to VPSDB";
                }
            }));
        }

        // Wait for all VPSDB threads to complete
        for (auto& future : vpsFutures) {
            try {
                future.get();
            } catch (const std::exception& e) {
                LOG_ERROR("VPinFileScanner: VPSDB thread exception: " << e.what());
            }
        }
    }

    LOG_INFO("VPinFileScanner: Completed vpin scanning and VPSDB enrichment.");
}