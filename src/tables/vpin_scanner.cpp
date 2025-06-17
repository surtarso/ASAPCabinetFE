// tables/vpin_scanner.cpp

#include "tables/vpin_scanner.h"
#include "tables/asap_index_manager.h"
#include "tables/vpsdb/vps_database_client.h"
#include "log/logging.h"
#include "vpin_wrapper.h" // For get_vpx_table_info_as_json and free_rust_string
#include "utils/string_utils.h"
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

    // Load VPSDB if enabled (Loaded before any parallel processing)
    VpsDatabaseClient vpsClient(settings.vpsDbPath);
    bool vpsLoaded = false;
    if (settings.fetchVPSdb) {
        if (progress) {
            std::lock_guard<std::mutex> lock(progress->mutex);
            progress->currentTask = "Fetching/Loading VPSDB (VPin scan phase)..."; // Update task for VPSDB initial load
        }
        if (vpsClient.fetchIfNeeded(settings.vpsDbLastUpdated, settings.vpsDbUpdateFrequency, progress) && vpsClient.load(progress)) {
            vpsLoaded = true;
            LOG_INFO("VPinFileScanner: VPSDB loaded successfully.");
        } else {
            LOG_ERROR("VPinFileScanner: Failed to load vpsdb.json, proceeding without it.");
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
    const size_t maxThreads = std::max(1u, std::thread::hardware_concurrency());

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
                    progress->numNoMatch++;
                    progress->logMessages.push_back("DEBUG: Failed to process: " + vpxFile);
                }
                free_rust_string(json_result); // Safely handles null
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

                // --- Populate File Metadata fields (from vpin's TableInfo struct) ---
                table.tableName = StringUtils::cleanMetadataString(StringUtils::safeGetMetadataString(vpinJson, "table_name", ""));
                table.tableAuthor = StringUtils::cleanMetadataString(StringUtils::safeGetMetadataString(vpinJson, "author_name", ""));
                table.tableDescription = StringUtils::cleanMetadataString(StringUtils::safeGetMetadataString(vpinJson, "table_description", ""));
                table.tableSaveDate = StringUtils::safeGetMetadataString(vpinJson, "table_save_date", "");
                table.tableReleaseDate = StringUtils::safeGetMetadataString(vpinJson, "release_date", "");
                table.tableVersion = StringUtils::cleanMetadataString(StringUtils::safeGetMetadataString(vpinJson, "table_version", ""));
                table.tableRevision = StringUtils::safeGetMetadataString(vpinJson, "table_save_rev", "");
                table.tableBlurb = StringUtils::cleanMetadataString(StringUtils::safeGetMetadataString(vpinJson, "table_blurb", ""));
                table.tableRules = StringUtils::cleanMetadataString(StringUtils::safeGetMetadataString(vpinJson, "table_rules", ""));
                table.tableAuthorEmail = StringUtils::cleanMetadataString(StringUtils::safeGetMetadataString(vpinJson, "author_email", ""));
                table.tableAuthorWebsite = StringUtils::cleanMetadataString(StringUtils::safeGetMetadataString(vpinJson, "author_website", ""));

                // --- Populate fields from 'properties' HashMap ---
                if (vpinJson.contains("properties") && vpinJson["properties"].is_object()) {
                    const json& properties = vpinJson["properties"];
                    
                    // TableType (e.g., "VPinMame")
                    table.tableType = StringUtils::cleanMetadataString(StringUtils::safeGetMetadataString(properties, "TableType", ""));
                    
                    // Company Name (check for "CompanyName" first, then "Company")
                    table.tableManufacturer = StringUtils::cleanMetadataString(StringUtils::safeGetMetadataString(properties, "CompanyName", 
                                                                 StringUtils::safeGetMetadataString(properties, "Company", "")));
                    
                    // Company Year (check for "CompanyYear" first, then "Year")
                    table.tableYear = StringUtils::cleanMetadataString(StringUtils::safeGetMetadataString(properties, "CompanyYear", 
                                                                 StringUtils::safeGetMetadataString(properties, "Year", "")));
                }

                // IMPORTANT: Do NOT overwrite `table.title`, `table.manufacturer`, `table.year`, `table.romName` here.
                // These are populated by FileScanner and serve as primary "local" identification for comparison.
                // The new `tableName`, `tableManufacturer`, `tableYear` hold the VPin internal data.
                // The `jsonOwner` is still correctly set here.
                table.jsonOwner = "VPin Filescan";

                LOG_DEBUG("VPinFileScanner: vpin table scanned: path=" << table.vpxFile 
                          << ", filename_title=" << table.title 
                          << ", vpin_table_name=" << table.tableName 
                          << ", filename_man=" << table.manufacturer 
                          << ", vpin_company_name=" << table.tableManufacturer 
                          << ", rom=" << table.romName);

                if (progress) {
                    std::lock_guard<std::mutex> lock(progress->mutex);
                    progress->numMatched++;
                    progress->logMessages.push_back("DEBUG: Processed: " + vpxFile);
                }
            } catch (const json::exception& e) {
                LOG_ERROR("VPinFileScanner: JSON parsing error for " << vpxFile << ": " << e.what());
                if (progress) {
                    std::lock_guard<std::mutex> lock(progress->mutex);
                    progress->numNoMatch++;
                    progress->logMessages.push_back("DEBUG: JSON error: " + vpxFile);
                }
            } catch (...) {
                LOG_ERROR("VPinFileScanner: Unexpected error processing " << vpxFile);
                if (progress) {
                    std::lock_guard<std::mutex> lock(progress->mutex);
                    progress->numNoMatch++;
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

    // Phase 2: VPSDB matchmaking (after initial VPin scan is complete for all tables)
    if (vpsLoaded) {
        // Reset progress counters for the VPSDB matching phase
        if (progress) {
            std::lock_guard<std::mutex> lock(progress->mutex);
            progress->currentTask = "Matching tables to VPSDB...";
            progress->currentTablesLoaded = 0;
            progress->totalTablesToLoad = tables.size();
            progress->numMatched = 0;
            progress->numNoMatch = 0;
            progress->logMessages.push_back("DEBUG: Starting VPSDB matchmaking for " + std::to_string(tables.size()) + " tables");
        }

        const size_t maxThreadsVps = std::max(1u, std::thread::hardware_concurrency());
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
                // Construct a temporary JSON object for VPSDB matching, using all available data
                json tempVpinJsonForVps;
                tempVpinJsonForVps["path"] = table.vpxFile;
                tempVpinJsonForVps["rom"] = table.romName; // romName comes from FileScanner (pinmame detection)

                tempVpinJsonForVps["table_info"] = {
                    // Use file metadata fields here for VPSDB matching
                    {"table_name", table.tableName},
                    {"author_name", table.tableAuthor},
                    {"table_description", table.tableDescription},
                    {"table_version", table.tableVersion},
                    {"table_save_date", table.tableSaveDate},
                    {"release_date", table.tableReleaseDate},
                    {"table_save_rev", table.tableRevision},
                    {"table_blurb", table.tableBlurb},
                    {"table_rules", table.tableRules}, 
                    {"author_email", table.tableAuthorEmail},
                    {"author_website", table.tableAuthorWebsite}
                };
                tempVpinJsonForVps["properties"] = {
                    {"manufacturer", table.tableManufacturer}, // manufacturer property
                    {"year", table.tableYear},       // year property
                    {"TableType", table.tableType}
                };

                // Add filename-derived fields explicitly for VPSDB to consider (if VPSDB client uses them)
                tempVpinJsonForVps["filename_title"] = table.title;
                tempVpinJsonForVps["filename_manufacturer"] = table.manufacturer;
                tempVpinJsonForVps["filename_year"] = table.year;


                vpsClient.matchMetadata(tempVpinJsonForVps, table, progress);

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

    LOG_INFO("VPinFileScanner: Completed vpin scanning and VPSDB matchmaking.");
}