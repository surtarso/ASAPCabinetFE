/**
 * @file vps_data_scanner.cpp
 * @brief Implements the VpsDataScanner class for enriching table data with VPSDB information.
 *
 * This file provides the implementation of the VpsDataScanner class, which enhances TableData objects with
 * metadata from a VPS database by matching VPX table data using string similarity and
 * metadata comparison. It integrates with LoadingProgress for progress tracking.
 */

#include "vps_data_scanner.h" // Your header for VpsDataScanner
#include "vps_utils.h"        // The utility class for string operations
#include <fstream>            // For file operations (mismatch log)
#include <regex>              // For regular expressions
#include <mutex>              // For std::mutex and std::lock_guard
#include <algorithm>          // For std::min, std::max
#include <filesystem>         // For std::filesystem::path (C++17)
#include <set>                // For std::set (used in word splitting)
#include <sstream>            // For std::istringstream
#include "utils/logging.h"    // Your logging utility

// Namespace alias for convenience if you're using C++17 or later
namespace fs = std::filesystem;

// Constructor definition
VpsDataScanner::VpsDataScanner(const nlohmann::json& vpsDb) : vpsDb_(vpsDb) {}

// Static mutex for thread-safe logging to the mismatch file
static std::mutex mismatchLogMutex;

// Helper function to split a string into normalized words for overlap checks
std::set<std::string> splitIntoWords(const std::string& str, const VpsUtils& utils) {
    std::set<std::string> words;
    // Normalize string using less aggressive rules, then split by whitespace
    std::string normalized = utils.normalizeStringLessAggressive(str);
    std::istringstream iss(normalized);
    std::string word;
    while (iss >> word) {
        if (!word.empty()) { // Ensure no empty words are inserted
            words.insert(word);
        }
    }
    return words;
}

// Check if there's any word overlap between two strings (case-insensitive, normalized)
bool hasWordOverlap(const std::string& str1, const std::string& str2, const VpsUtils& utils) {
    std::set<std::string> words1 = splitIntoWords(str1, utils);
    std::set<std::string> words2 = splitIntoWords(str2, utils);

    // If either string is empty, there's no overlap
    if (words1.empty() || words2.empty()) {
        return false;
    }

    for (const auto& word : words1) {
        if (words2.count(word) > 0) {
            return true; // Found at least one common word
        }
    }
    return false; // No common words found
}

// Levenshtein distance calculation
size_t VpsDataScanner::levenshteinDistance(const std::string& s1, const std::string& s2) const {
    const size_t len1 = s1.size();
    const size_t len2 = s2.size();

    // Handle empty string cases
    if (len1 == 0) return len2;
    if (len2 == 0) return len1;

    std::vector<std::vector<size_t>> dp(len1 + 1, std::vector<size_t>(len2 + 1));

    // Initialize DP table
    for (size_t i = 0; i <= len1; ++i) dp[i][0] = i;
    for (size_t j = 0; j <= len2; ++j) dp[0][j] = j;

    // Fill DP table
    for (size_t i = 1; i <= len1; ++i) {
        for (size_t j = 1; j <= len2; ++j) {
            size_t cost = (s1[i - 1] == s2[j - 1]) ? 0 : 1;
            dp[i][j] = std::min({dp[i - 1][j] + 1,      // Deletion
                                 dp[i][j - 1] + 1,      // Insertion
                                 dp[i - 1][j - 1] + cost}); // Substitution
        }
    }
    return dp[len1][len2];
}

// Main metadata matching function
bool VpsDataScanner::matchMetadata(const nlohmann::json& vpxTable, TableData& tableData, LoadingProgress* progress) const {
    LOG_DEBUG("Starting enrichTableData for table path: " << vpxTable.value("path", "N/A") << ", tableName=" << tableData.tableName << ", manufacturer=" << tableData.manufacturer << ", year=" << tableData.year);

    // Initial logging for debugging, can be removed in production
    {
        std::lock_guard<std::mutex> lock(mismatchLogMutex);
        std::ofstream testLog("tables/scan_debug.log", std::ios::app); // Changed name to avoid confusion with mismatches
        testLog << "Processing table: " << vpxTable.value("path", "N/A") << "\n";
    }

    if (!vpxTable.is_object()) {
        LOG_DEBUG("vpxTable is not an object. Skipping enrichment for: " << vpxTable.dump()); // Dump for more context
        return false;
    }

    std::string filename_full_path = vpxTable.value("path", "");
    std::string filename = "";
    if (!filename_full_path.empty()) {
        filename = fs::path(filename_full_path).stem().string();
    }

    // --- Populate TableData from VPX file metadata first ---
    // Prioritize existing `tableData` fields if they came from a more reliable source
    // (e.g., vpin scanner) unless filename has more relevant data.

    // Extract table_name from vpxTable, preferring it if it has word overlap with filename
    std::string vpxTable_tableName_from_metadata;
    if (vpxTable.contains("table_info") && vpxTable["table_info"].is_object()) {
        const auto& tableInfo = vpxTable["table_info"];
        vpxTable_tableName_from_metadata = tableInfo.value("table_name", "");

        // Set tableData.tableName:
        // Prefer `vpxTable_tableName_from_metadata` if it's not empty AND
        // it has a significant word overlap with the filename.
        // Otherwise, fallback to filename if `tableData.tableName` is currently empty.
        if (!vpxTable_tableName_from_metadata.empty() && hasWordOverlap(vpxTable_tableName_from_metadata, filename, utils_)) {
            tableData.tableName = vpxTable_tableName_from_metadata;
            LOG_DEBUG("Set tableData.tableName from vpxTable metadata (with filename overlap): " << tableData.tableName);
        } else if (tableData.tableName.empty() && !filename.empty()) {
            tableData.tableName = filename;
            LOG_DEBUG("Set tableData.tableName from filename (no metadata overlap or empty): " << tableData.tableName);
        } else if (tableData.tableName.empty() && !vpxTable_tableName_from_metadata.empty()) {
            tableData.tableName = vpxTable_tableName_from_metadata; // Fallback if no filename and metadata is available
            LOG_DEBUG("Set tableData.tableName from vpxTable metadata (no filename): " << tableData.tableName);
        }
        // If tableData.tableName already has a value (e.g., from a previous scanner), keep it.

        // Populate other tableInfo fields if currently empty
        if (tableData.authorName.empty()) {
            tableData.authorName = tableInfo.value("author_name", "");
            LOG_DEBUG("Set authorName from vpxTable: " << tableData.authorName);
        }
        if (tableData.tableDescription.empty()) {
            tableData.tableDescription = tableInfo.value("table_description", "");
            LOG_DEBUG("Set tableDescription from vpxTable: " << tableData.tableDescription);
        }
        if (tableData.tableVersion.empty()) {
            if (tableInfo.contains("table_version") && !tableInfo["table_version"].is_null()) {
                tableData.tableVersion = tableInfo["table_version"].is_string() ? tableInfo["table_version"].get<std::string>() :
                                         std::to_string(tableInfo["table_version"].get<double>());
                LOG_DEBUG("Set tableVersion from vpxTable: " << tableData.tableVersion);
            }
        }
    }

    if (tableData.romName.empty()) {
        tableData.romName = vpxTable.value("game_name", ""); // This usually comes from VPX file
        if (!tableData.romName.empty()) {
            tableData.romPath = tableData.romName; // romPath is derived from romName in this context
            LOG_DEBUG("Set romName and romPath from vpxTable: " << tableData.romName);
        }
    }

    // --- Extract Year and Manufacturer from `tableData.tableName` or `filename` ---
    // Use `VpsUtils` for robust year extraction. The regex logic in `extractYearFromDate` is superior.
    if (tableData.year.empty() && !tableData.tableName.empty()) {
        tableData.year = utils_.extractYearFromDate(tableData.tableName);
        if (!tableData.year.empty()) {
            LOG_DEBUG("Extracted year from tableData.tableName: " << tableData.year);
        }
    }
    if (tableData.year.empty() && !filename.empty()) { // Fallback to filename if tableName didn't yield a year
        tableData.year = utils_.extractYearFromDate(filename);
        if (!tableData.year.empty()) {
            LOG_DEBUG("Extracted year from filename: " << tableData.year);
        }
    }

    // Manufacturer extraction (from parentheses) - this remains a common heuristic
    if (tableData.manufacturer.empty() && !tableData.tableName.empty()) {
        std::regex manufRegex(R"(\(([^)]+?)(?:\s+(?:19|20)\d{2})?\))"); // Improved regex to avoid capturing year if present
        std::smatch match;
        if (std::regex_search(tableData.tableName, match, manufRegex) && match.size() > 1) {
            tableData.manufacturer = match[1].str();
            LOG_DEBUG("Extracted manufacturer from tableData.tableName: " << tableData.manufacturer);
        }
    }
    if (tableData.manufacturer.empty() && !filename.empty()) { // Fallback to filename
        std::regex manufRegex(R"(\(([^)]+?)(?:\s+(?:19|20)\d{2})?\))");
        std::smatch match;
        if (std::regex_search(filename, match, manufRegex) && match.size() > 1) {
            tableData.manufacturer = match[1].str();
            LOG_DEBUG("Extracted manufacturer from filename: " << tableData.manufacturer);
        }
    }


    // --- VPSDB Matching Logic ---
    bool matched_to_vpsdb = false;
    nlohmann::json bestVpsDbEntry;
    std::string latestVpsVersionFound;
    std::string bestMatchVpsName;
    float bestMatchScore = -1.0f; // Initialize with a very low score

    // Prepare normalized strings for comparison to avoid redundant calls
    std::string normTableNameAggressive = utils_.normalizeString(tableData.tableName);
    std::string normTableNameLessAggressive = utils_.normalizeStringLessAggressive(tableData.tableName);
    std::string normGameName = utils_.normalizeString(tableData.romName);
    std::string normFilenameLessAggressive = utils_.normalizeStringLessAggressive(filename);

    LOG_DEBUG("Attempting to match table: tableName='" << tableData.tableName << "', romName='" << tableData.romName << "', filename='" << filename << "'");

    size_t totalVpsEntries = vpsDb_.size();
    size_t processedVpsEntries = 0;

    if (progress) {
        std::lock_guard<std::mutex> lock(progress->mutex);
        progress->currentTask = "Matching VPSDB " + std::to_string(totalVpsEntries) + " entries...";
        // Note: progress->currentTablesLoaded should probably track progress of the outer loop (total VPX tables being scanned)
        // This 'processedVpsEntries' variable is for internal progress within this function, if needed for a sub-progress bar.
        // For the main progress bar, you'd update progress->currentTablesLoaded outside this function.
    }

    for (const auto& vpsDbEntry : vpsDb_) {
        // Increment processed entries here to ensure it counts all iterations
        processedVpsEntries++;
        if (progress) {
            std::lock_guard<std::mutex> lock(progress->mutex);
            // Update an internal sub-progress if you have one, or just update overall
            // For now, let's keep it simple and assume the main progress tracks tables.
            // progress->currentTablesLoaded = processedVpsEntries; // This might be confusing if used for overall tables
        }

        try {
            if (!vpsDbEntry.is_object()) {
                LOG_DEBUG("Skipping non-object VPSDB entry.");
                continue;
            }

            std::string vpsId = vpsDbEntry.value("id", "N/A");
            std::string vpsName = vpsDbEntry.value("name", "");
            std::string vpsYear;
            std::string vpsManufacturer = vpsDbEntry.value("manufacturer", "");
            std::string currentVpsEntryLatestVersion;

            if (vpsDbEntry.contains("year") && !vpsDbEntry["year"].is_null()) {
                vpsYear = vpsDbEntry["year"].is_number_integer() ? std::to_string(vpsDbEntry["year"].get<int>()) :
                          vpsDbEntry["year"].is_string() ? vpsDbEntry["year"].get<std::string>() : "";
            }
            if (vpsName.empty()) {
                LOG_DEBUG("Skipping VPSDB entry with empty name, ID: " << vpsId);
                continue;
            }

            // Determine the latest VPX version from tableFiles for the current VPSDB entry
            if (vpsDbEntry.contains("tableFiles") && vpsDbEntry["tableFiles"].is_array()) {
                for (const auto& file : vpsDbEntry["tableFiles"]) {
                    if (file.is_object() && file.value("tableFormat", "") == "VPX") {
                        std::string vpsFileVersion = file.value("version", "");
                        if (utils_.isVersionGreaterThan(vpsFileVersion, currentVpsEntryLatestVersion)) {
                            currentVpsEntryLatestVersion = vpsFileVersion;
                        }
                    }
                }
            }

            std::string normVpsNameAggressive = utils_.normalizeString(vpsName);
            std::string normVpsNameLessAggressive = utils_.normalizeStringLessAggressive(vpsName);
            float currentMatchScore = 0.0f;

            // --- Match by ROM Name (highest priority, direct match) ---
            if (!normGameName.empty() && vpsDbEntry.contains("tableFiles") && vpsDbEntry["tableFiles"].is_array()) {
                for (const auto& tableFile : vpsDbEntry["tableFiles"]) {
                    if (tableFile.is_object() && tableFile.contains("roms") && tableFile["roms"].is_array()) {
                        for (const auto& rom : tableFile["roms"]) {
                            if (rom.is_object() && rom.contains("name") && rom["name"].is_string()) {
                                std::string romNameFromVps = rom["name"].get<std::string>();
                                if (!romNameFromVps.empty() && utils_.normalizeString(romNameFromVps) == normGameName) {
                                    currentMatchScore += 10.0f; // High score for ROM match
                                    LOG_DEBUG("ROM name exact match for: " << tableData.romName << " with VPSDB entry: " << vpsName);
                                    goto end_rom_check; // Break out of nested loops
                                }
                            }
                        }
                    }
                }
            }
            end_rom_check:; // Label for goto

            // --- Match by Table Name / Filename Similarity ---
            float nameSimilarityTableName = 0.0f;
            float nameSimilarityFilename = 0.0f;

            // Try to match `tableData.tableName`
            if (!normTableNameAggressive.empty() && normTableNameAggressive == normVpsNameAggressive) {
                nameSimilarityTableName = 3.0f;
                LOG_DEBUG("Exact normalized tableName match: " << tableData.tableName);
            } else if (!normTableNameLessAggressive.empty() && normTableNameLessAggressive == normVpsNameLessAggressive) {
                nameSimilarityTableName = 2.5f; // Slightly less for less aggressive match
                LOG_DEBUG("Less aggressive normalized tableName match: " << tableData.tableName);
            } else if (!normTableNameLessAggressive.empty()) {
                size_t dist = levenshteinDistance(normTableNameLessAggressive, normVpsNameLessAggressive);
                float similarity = 1.0f - static_cast<float>(dist) / std::max(normTableNameLessAggressive.size(), normVpsNameLessAggressive.size());
                if (similarity >= 0.7f) { // Only consider if similarity is reasonably high
                    nameSimilarityTableName = similarity * 2.0f; // Scale score by similarity
                    LOG_DEBUG("Levenshtein tableName match, similarity=" << similarity << ", score=" << nameSimilarityTableName);
                }
            }

            // Try to match `filename`
            if (!normFilenameLessAggressive.empty()) {
                if (normFilenameLessAggressive == normVpsNameLessAggressive) {
                    nameSimilarityFilename = 3.5f; // Slightly higher priority for filename exact match
                    LOG_DEBUG("Exact normalized filename match: " << filename << ", score=" << nameSimilarityFilename);
                } else {
                    size_t dist = levenshteinDistance(normFilenameLessAggressive, normVpsNameLessAggressive);
                    float similarity = 1.0f - static_cast<float>(dist) / std::max(normFilenameLessAggressive.size(), normVpsNameLessAggressive.size());
                    if (similarity >= 0.7f) {
                        nameSimilarityFilename = similarity * 3.0f; // Scale and give higher weight
                        LOG_DEBUG("Levenshtein filename match, similarity=" << similarity << ", score=" << nameSimilarityFilename);
                    }
                }
            }
            
            // Add the best name similarity score to the current match score
            currentMatchScore += std::max(nameSimilarityTableName, nameSimilarityFilename);
            if (nameSimilarityFilename > nameSimilarityTableName) {
                LOG_DEBUG("Filename match (`" << filename << "`) scored better than tableName (`" << tableData.tableName << "`).");
            }


            // --- Match by Year ---
            // Normalize VPS year using VpsUtils::extractYearFromDate for robustness
            std::string normalizedVpsYear = utils_.extractYearFromDate(vpsYear);
            if (!tableData.year.empty() && !normalizedVpsYear.empty() && tableData.year == normalizedVpsYear) {
                currentMatchScore += 1.5f; // Year is a strong indicator
                LOG_DEBUG("Year match: Table Year=" << tableData.year << ", VPS Year=" << normalizedVpsYear);
            }


            // --- Match by Manufacturer ---
            if (!tableData.manufacturer.empty() && !vpsManufacturer.empty() &&
                utils_.normalizeStringLessAggressive(tableData.manufacturer) == utils_.normalizeStringLessAggressive(vpsManufacturer)) {
                currentMatchScore += 1.0f; // Manufacturer is a good indicator
                LOG_DEBUG("Manufacturer match: Table Manufacturer='" << tableData.manufacturer << "', VPS Manufacturer='" << vpsManufacturer << "'");
            }
            
            // --- Determine if this is the best match found so far ---
            // Prioritize higher score, then higher version if scores are tied
            if (currentMatchScore > bestMatchScore) {
                bestMatchScore = currentMatchScore;
                bestVpsDbEntry = vpsDbEntry;
                latestVpsVersionFound = currentVpsEntryLatestVersion;
                bestMatchVpsName = vpsName;
                matched_to_vpsdb = true;
                LOG_DEBUG("New best match found! Score: " << bestMatchScore << ", VPS Name: '" << vpsName << "'");
            } else if (currentMatchScore == bestMatchScore && !latestVpsVersionFound.empty() &&
                       utils_.isVersionGreaterThan(currentVpsEntryLatestVersion, latestVpsVersionFound)) {
                // If scores are equal, prefer the entry with a newer VPS version
                bestMatchScore = currentMatchScore; // Keep the same score
                bestVpsDbEntry = vpsDbEntry;
                latestVpsVersionFound = currentVpsEntryLatestVersion;
                bestMatchVpsName = vpsName;
                matched_to_vpsdb = true;
                LOG_DEBUG("Updated best match due to newer VPS version! Score: " << bestMatchScore << ", VPS Name: '" << vpsName << "'");
            }

        } catch (const nlohmann::json::exception& e) {
            LOG_DEBUG("JSON parsing error for a VPSDB entry: " << e.what());
            continue;
        } catch (const std::exception& e) {
            LOG_ERROR("An unexpected error occurred processing a VPSDB entry: " << e.what());
            continue;
        }
    }

    // --- Apply best match to TableData ---
    if (matched_to_vpsdb) {
        tableData.vpsId = bestVpsDbEntry.value("id", "");
        tableData.vpsName = bestMatchVpsName; // Use the vpsName of the best matched entry
        tableData.vpsType = bestVpsDbEntry.value("type", "");
        tableData.vpsThemes = bestVpsDbEntry.contains("theme") && bestVpsDbEntry["theme"].is_array() ? utils_.join(bestVpsDbEntry["theme"], ", ") : "";
        tableData.vpsDesigners = bestVpsDbEntry.contains("designers") && bestVpsDbEntry["designers"].is_array() ? utils_.join(bestVpsDbEntry["designers"], ", ") : "";
        tableData.vpsPlayers = bestVpsDbEntry.contains("players") && !bestVpsDbEntry["players"].is_null() && bestVpsDbEntry["players"].is_number_integer() ? std::to_string(bestVpsDbEntry["players"].get<int>()) : "";
        tableData.vpsIpdbUrl = bestVpsDbEntry.value("ipdbUrl", "");
        tableData.vpsManufacturer = bestVpsDbEntry.value("manufacturer", ""); // Direct copy from VPSDB
        tableData.vpsYear = bestVpsDbEntry.contains("year") && !bestVpsDbEntry["year"].is_null() ?
                            (bestVpsDbEntry["year"].is_number_integer() ? std::to_string(bestVpsDbEntry["year"].get<int>()) :
                             bestVpsDbEntry["year"].is_string() ? bestVpsDbEntry["year"].get<std::string>() : "") : "";

        // Iterate through tableFiles to find relevant VPX-specific metadata
        if (bestVpsDbEntry.contains("tableFiles") && bestVpsDbEntry["tableFiles"].is_array()) {
            for (const auto& file : bestVpsDbEntry["tableFiles"]) {
                if (file.is_object() && file.value("tableFormat", "") == "VPX") {
                    tableData.vpsAuthors = file.contains("authors") && file["authors"].is_array() ? utils_.join(file["authors"], ", ") : "";
                    tableData.vpsFeatures = file.contains("features") && file["features"].is_array() ? utils_.join(file["features"], ", ") : "";
                    tableData.vpsComment = file.value("comment", ""); // Store VPS comment directly

                    // Prefer VPS comment for tableDescription if it's more descriptive or current `tableDescription` is empty
                    if (!tableData.vpsComment.empty() && (tableData.tableDescription.empty() || tableData.vpsComment.length() > tableData.tableDescription.length())) {
                        tableData.tableDescription = tableData.vpsComment;
                        LOG_DEBUG("Updated tableDescription from VPSDB comment: " << tableData.tableDescription);
                    }
                    // Extract imgUrl and tableUrl from the first relevant tableFile entry
                    if (file.contains("imgUrl") && file["imgUrl"].is_string()) {
                        tableData.vpsImgUrl = file["imgUrl"].get<std::string>();
                    }
                    if (file.contains("urls") && file["urls"].is_array() && !file["urls"].empty() && file["urls"][0].is_object() && file["urls"][0].contains("url")) {
                        tableData.vpsTableUrl = file["urls"][0]["url"].get<std::string>();
                    }
                    break; // Assuming the first VPX tableFile is sufficient for these common fields
                }
            }
        }

        // Update `tableData.manufacturer` if VPSDB provides a value and it's better
        if (!tableData.vpsManufacturer.empty() && tableData.manufacturer.empty()) {
            tableData.manufacturer = tableData.vpsManufacturer;
            LOG_DEBUG("Updated manufacturer from VPSDB: " << tableData.manufacturer);
        } else if (!tableData.vpsManufacturer.empty() && !tableData.manufacturer.empty() &&
                   utils_.normalizeStringLessAggressive(tableData.vpsManufacturer) != utils_.normalizeStringLessAggressive(tableData.manufacturer) &&
                   bestMatchScore >= 5.0f) { // Arbitrary threshold for when to override
            LOG_DEBUG("VPSDB manufacturer '" << tableData.vpsManufacturer << "' differs from existing '" << tableData.manufacturer << "'. Considering override based on score.");
            // Logic to decide if VPSDB manufacturer should override existing.
            // For now, if initial matching was strong, we could favor VPSDB.
            // You might want a more sophisticated merge/override strategy here.
            tableData.manufacturer = tableData.vpsManufacturer; // Override for strong matches
            LOG_DEBUG("Overriding manufacturer with VPSDB value: " << tableData.manufacturer);
        }


        // Update `tableData.year` if VPSDB provides a value and it's better
        if (!tableData.vpsYear.empty() && tableData.year.empty()) {
            tableData.year = tableData.vpsYear;
            LOG_DEBUG("Updated year from VPSDB: " << tableData.year);
        } else if (!tableData.vpsYear.empty() && !tableData.year.empty() &&
                   tableData.vpsYear != tableData.year && bestMatchScore >= 5.0f) {
            LOG_DEBUG("VPSDB year '" << tableData.vpsYear << "' differs from existing '" << tableData.year << "'. Considering override based on score.");
            tableData.year = tableData.vpsYear; // Override for strong matches
            LOG_DEBUG("Overriding year with VPSDB value: " << tableData.year);
        }

        // Determine the `title` to display
        // Prefer `vpsName` if match confidence is high, otherwise use `tableData.tableName` or `filename`
        if (bestMatchScore >= 5.0f) { // If the match is strong, use the VPS name for the title
            tableData.title = tableData.vpsName;
            LOG_DEBUG("Title updated to VPSDB name due to strong match: " << tableData.title);
        } else {
            // Otherwise, keep the best local title (tableName or filename)
            tableData.title = tableData.tableName.empty() ? filename : tableData.tableName;
            LOG_DEBUG("Title kept as local (tableName/filename) due to weaker match: " << tableData.title);
        }

        // Update `tableVersion` with latest VPS version if it's newer
        std::string currentVpxVersionNormalized = utils_.normalizeVersion(tableData.tableVersion);
        tableData.vpsVersion = latestVpsVersionFound; // Store the latest VPS version found for the best match
        
        if (!latestVpsVersionFound.empty() && utils_.isVersionGreaterThan(latestVpsVersionFound, currentVpxVersionNormalized)) {
            // Append latest VPS version if existing is older or empty
            if (!currentVpxVersionNormalized.empty()) {
                tableData.tableVersion = currentVpxVersionNormalized + " (Latest VPS: " + latestVpsVersionFound + ")";
            } else {
                tableData.tableVersion = "(Latest VPS: " + latestVpsVersionFound + ")";
            }
            LOG_DEBUG("Updated tableVersion with latest VPS version: " << tableData.tableVersion);
        } else if (tableData.tableVersion.empty() && !latestVpsVersionFound.empty()) {
            // If local version is empty, just use the VPS latest version
            tableData.tableVersion = latestVpsVersionFound;
            LOG_DEBUG("Set tableVersion to VPS latest version (local was empty): " << tableData.tableVersion);
        }
        // If local version is newer or equal, keep it as is.


        // Set confidence and owner
        tableData.matchConfidence = std::min(bestMatchScore / 10.0f, 1.0f); // Cap at 1.0
        tableData.matchScore = tableData.matchConfidence; // Match score for display
        tableData.jsonOwner = "VPSDB"; // Indicate that this data was enriched by VPSDB
        LOG_INFO("Successfully matched table to VPSDB, confidence: " << tableData.matchScore << " for: " << tableData.title);
        
        if (progress) {
            std::lock_guard<std::mutex> lock(progress->mutex);
            progress->numMatched++; // Increment overall matched count
        }
    } else {
        // --- No VPSDB match found ---
        // Ensure title is set even if no match
        tableData.title = tableData.tableName.empty() ? filename : tableData.tableName;

        // Log mismatches to a dedicated file
        {
            std::lock_guard<std::mutex> lock(mismatchLogMutex);
            std::ofstream mismatchLog("tables/vpsdb_mismatches.log", std::ios::app);
            mismatchLog << "No VPSDB match for table: '" << tableData.title
                        << "', ROM Name: '" << tableData.romName
                        << "', Filename: '" << filename << "'\n";
            mismatchLog.close(); // Close immediately to ensure write
        }
        LOG_INFO("No VPSDB match found for table: " << tableData.title);

        if (progress) {
            std::lock_guard<std::mutex> lock(progress->mutex);
            progress->numNoMatch++; // Increment overall no-match count
        }
    }

    LOG_DEBUG("Final TableData for '" << filename << "': Title='" << tableData.title
              << "', Manufacturer='" << tableData.manufacturer
              << "', Year='" << tableData.year
              << "', VPS ID='" << tableData.vpsId << "'");
    return matched_to_vpsdb;
}