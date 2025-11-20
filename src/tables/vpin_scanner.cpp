#include "vpin_scanner.h"
#include "log/logging.h"
#include "vpin_wrapper.h"
#include "utils/string_utils.h"
#include <filesystem>
#include <nlohmann/json.hpp>
#include <thread>
#include <future>
#include <vector>
#include <atomic>

namespace fs = std::filesystem;
using json = nlohmann::json;

// void VPinScanner::scanFiles(const Settings& settings, std::vector<TableData>& tables, LoadingProgress* progress) {
void VPinScanner::scanFiles(std::vector<TableData>& tables, LoadingProgress* progress) {
    LOG_DEBUG("Starting scan with vpin for " + std::to_string(tables.size()) + " tables.");

    if (progress) {
        std::lock_guard<std::mutex> lock(progress->mutex);
        progress->currentTask = "Scanning VPX files with vpin...";
        progress->totalTablesToLoad = tables.size();
        progress->currentTablesLoaded = 0;
        progress->numMatched = 0;
        progress->numNoMatch = 0;
        progress->logMessages.push_back("DEBUG: Initialized vpin scan for " + std::to_string(tables.size()) + " tables");
    }

    std::vector<std::future<void>> futures;
    std::atomic<int> processedVpin(0);
    const size_t maxThreads = std::max(1u, std::thread::hardware_concurrency());

    for (auto& table : tables) {
        while (futures.size() >= maxThreads) {
            for (auto it = futures.begin(); it != futures.end();) {
                if (it->wait_for(std::chrono::milliseconds(0)) == std::future_status::ready) {
                    try {
                        it->get();
                    } catch (const std::exception& e) {
                        LOG_ERROR("Thread exception during VPin scan for " + table.vpxFile + ": " + std::string(e.what()));
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
            LOG_DEBUG("Processing VPX file with VPin: " + vpxFile);
            char* json_result = get_vpx_table_info_as_json(vpxFile.c_str());
            if (!json_result) {
                LOG_ERROR("Failed to get metadata for " + vpxFile);
                if (progress) {
                    std::lock_guard<std::mutex> lock(progress->mutex);
                    progress->numNoMatch++;
                    progress->logMessages.push_back("DEBUG: Failed to process: " + vpxFile);
                }
                free_rust_string(json_result);
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
                table.tableLastModified = StringUtils::safeGetMetadataString(vpinJson, "last_modified", "");

                // TODO: check if this is from file or extracted from script in vpxtool, this is a dirty copy paste job.
                // table.tableRom = StringUtils::cleanMetadataString(StringUtils::safeGetMetadataString(vpinJson, "game_name"));


                if (vpinJson.contains("properties") && vpinJson["properties"].is_object()) {
                    const json& properties = vpinJson["properties"];
                    table.tableType = StringUtils::cleanMetadataString(StringUtils::safeGetMetadataString(properties, "TableType", ""));
                    table.tableManufacturer = StringUtils::cleanMetadataString(StringUtils::safeGetMetadataString(properties, "CompanyName",
                                                                 StringUtils::safeGetMetadataString(properties, "Company",
                                                                    StringUtils::safeGetMetadataString(properties, "manufacturer", ""))));
                    table.tableYear = StringUtils::cleanMetadataString(StringUtils::safeGetMetadataString(properties, "CompanyYear",
                                                                 StringUtils::safeGetMetadataString(properties, "Year", "")));
                }

                table.jsonOwner = "VPin Filescan";

                if (progress) {
                    std::lock_guard<std::mutex> lock(progress->mutex);
                    progress->numMatched++;
                    progress->logMessages.push_back("DEBUG: Processed: " + vpxFile);
                }
            } catch (const json::exception& e) {
                LOG_ERROR("JSON parsing error for " + vpxFile + ": " + e.what());
                if (progress) {
                    std::lock_guard<std::mutex> lock(progress->mutex);
                    progress->numNoMatch++;
                    progress->logMessages.push_back("DEBUG: JSON error: " + vpxFile);
                }
            } catch (...) {
                LOG_ERROR("Unexpected error processing " + vpxFile);
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

    for (auto& future : futures) {
        try {
            future.get();
        } catch (const std::exception& e) {
            LOG_ERROR("Thread exception: " + std::string(e.what()));
        }
    }

    LOG_INFO("Scan Completed.");
}
