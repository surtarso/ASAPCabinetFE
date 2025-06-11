#include "vps_data_scanner.h"
#include "vps_utils.h"
#include <fstream>
#include <regex>
#include <mutex>
#include <algorithm>
#include <filesystem>
#include <set>
#include <sstream>
#include "utils/logging.h"

namespace fs = std::filesystem;

VpsDataScanner::VpsDataScanner(const nlohmann::json& vpsDb) : vpsDb_(vpsDb) {}

static std::mutex mismatchLogMutex;

// Helper function to split string into words
std::set<std::string> splitIntoWords(const std::string& str, const VpsUtils& utils) {
    std::set<std::string> words;
    std::string normalized = utils.normalizeStringLessAggressive(str);
    std::istringstream iss(normalized);
    std::string word;
    while (iss >> word) {
        words.insert(word);
    }
    return words;
}

// Check if there’s any word overlap between two strings
bool hasWordOverlap(const std::string& str1, const std::string& str2, const VpsUtils& utils) {
    std::set<std::string> words1 = splitIntoWords(str1, utils);
    std::set<std::string> words2 = splitIntoWords(str2, utils);
    for (const auto& word : words1) {
        if (words2.count(word) > 0) {
            return true;
        }
    }
    return false;
}

size_t VpsDataScanner::levenshteinDistance(const std::string& s1, const std::string& s2) const {
    const size_t len1 = s1.size(), len2 = s2.size();
    std::vector<std::vector<size_t>> dp(len1 + 1, std::vector<size_t>(len2 + 1));
    for (size_t i = 0; i <= len1; ++i) dp[i][0] = i;
    for (size_t j = 0; j <= len2; ++j) dp[0][j] = j;
    for (size_t i = 1; i <= len1; ++i) {
        for (size_t j = 1; j <= len2; ++j) {
            size_t cost = (s1[i - 1] == s2[j - 1]) ? 0 : 1;
            dp[i][j] = std::min({dp[i - 1][j] + 1, dp[i][j - 1] + 1, dp[i - 1][j - 1] + cost});
        }
    }
    return dp[len1][len2];
}

bool VpsDataScanner::enrichTableData(const nlohmann::json& vpxTable, TableData& tableData, LoadingProgress* progress) const {
    LOG_DEBUG("Starting enrichTableData for table path: " << vpxTable.value("path", "N/A") << ", tableName=" << tableData.tableName << ", manufacturer=" << tableData.manufacturer << ", year=" << tableData.year);

    {
        std::lock_guard<std::mutex> lock(mismatchLogMutex);
        std::ofstream testLog("tables/test_log.txt", std::ios::app);
        testLog << "Test log entry for table: " << vpxTable.value("path", "N/A") << "\n";
    }

    if (!vpxTable.is_object()) {
        LOG_DEBUG("vpxTable is not an object, type: " << vpxTable.type_name());
        return false;
    }

    std::string filename;
    if (vpxTable.contains("path") && vpxTable["path"].is_string()) {
        filename = fs::path(vpxTable["path"].get<std::string>()).stem().string();
    }

    std::string vpxTableName_source;
    bool useMetadataTableName = false;
    if (vpxTable.contains("table_info") && vpxTable["table_info"].is_object()) {
        vpxTableName_source = vpxTable["table_info"].value("table_name", "");
        if (!vpxTableName_source.empty() && !filename.empty() && hasWordOverlap(vpxTableName_source, filename, utils_)) {
            useMetadataTableName = true;
            LOG_DEBUG("Metadata table name has overlap with filename: tableName=" << vpxTableName_source << ", filename=" << filename);
        } else {
            LOG_DEBUG("No overlap or empty metadata, preferring filename: tableName=" << vpxTableName_source << ", filename=" << filename);
        }
    }

    // Update tableData, default to filename unless metadata has overlap
    try {
        if (vpxTable.contains("table_info") && vpxTable["table_info"].is_object()) {
            const auto& tableInfo = vpxTable["table_info"];
            tableData.tableName = useMetadataTableName ? vpxTableName_source : filename;
            LOG_DEBUG("Set tableName to: " << tableData.tableName);
            if (tableData.authorName.empty()) {
                tableData.authorName = tableInfo.value("author_name", "");
                LOG_DEBUG("Set authorName from vpxTable: " << tableData.authorName);
            }
            if (tableData.tableDescription.empty()) {
                tableData.tableDescription = tableInfo.value("table_description", "");
                LOG_DEBUG("Set tableDescription from vpxTable: " << tableData.tableDescription);
            }
            if (tableData.tableVersion.empty()) {
                tableData.tableVersion = tableInfo.contains("table_version") && !tableInfo["table_version"].is_null() ?
                    (tableInfo["table_version"].is_string() ? tableInfo["table_version"].get<std::string>() :
                     std::to_string(tableInfo["table_version"].get<double>())) : "";
                LOG_DEBUG("Set tableVersion from vpxTable: " << tableData.tableVersion);
            }
        }
        if (tableData.gameName.empty()) {
            tableData.gameName = vpxTable.value("game_name", "");
            tableData.romPath = tableData.gameName;
            LOG_DEBUG("Set gameName and romPath from vpxTable: " << tableData.gameName);
        }

        // Extract year and manufacturer from tableName or filename
        if (tableData.year.empty() && !tableData.tableName.empty()) {
            std::regex yearRegex(R"(\b(\d{4})\b)");
            std::smatch match;
            if (std::regex_search(tableData.tableName, match, yearRegex)) {
                tableData.year = match[1].str();
                LOG_DEBUG("Extracted year from tableName: " << tableData.year);
            }
        }
        if (tableData.manufacturer.empty() && !tableData.tableName.empty()) {
            std::regex manufRegex(R"(\(([^)]+?)(?:\s+\d{4})?\))");
            std::smatch match;
            if (std::regex_search(tableData.tableName, match, manufRegex)) {
                tableData.manufacturer = match[1].str();
                LOG_DEBUG("Extracted manufacturer from tableName: " << tableData.manufacturer);
            }
        }
        if (!filename.empty()) {
            if (tableData.year.empty()) {
                std::regex yearRegex(R"(\b(\d{4})\b)");
                std::smatch match;
                if (std::regex_search(filename, match, yearRegex)) {
                    tableData.year = match[1].str();
                    LOG_DEBUG("Extracted year from filename: " << tableData.year);
                }
            }
            if (tableData.manufacturer.empty()) {
                std::regex manufRegex(R"(\(([^)]+?)(?:\s+\d{4})?\))");
                std::smatch match;
                if (std::regex_search(filename, match, manufRegex)) {
                    tableData.manufacturer = match[1].str();
                    LOG_DEBUG("Extracted manufacturer from filename: " << tableData.manufacturer);
                }
            }
        }
    } catch (const std::exception& e) {
        LOG_DEBUG("Error processing vpxTable: " << e.what());
    }

    bool matched_to_vpsdb = false;
    nlohmann::json bestVpsDbEntry;
    std::string latestVpsVersionFound;
    std::string bestMatchVpsName;
    float bestMatchScore = -1.0f;

    std::string normTableNameAggressive = utils_.normalizeString(tableData.tableName);
    std::string normTableNameLessAggressive = utils_.normalizeStringLessAggressive(tableData.tableName);
    std::string normGameName = utils_.normalizeString(tableData.gameName);
    std::string normFilename = utils_.normalizeStringLessAggressive(filename);

    LOG_DEBUG("Attempting to match table: tableName=" << tableData.tableName << ", gameName=" << tableData.gameName << ", filename=" << filename);

    size_t totalVpsEntries = vpsDb_.size();
    size_t processedVpsEntries = 0;

    if (progress) {
        std::lock_guard<std::mutex> lock(progress->mutex);
        progress->currentTask = "Matching VPSDB " + std::to_string(totalVpsEntries) + " entries...";
        progress->currentTablesLoaded = 0;
    }

    for (const auto& vpsDbEntry : vpsDb_) {
        try {
            if (!vpsDbEntry.is_object()) continue;

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

            if (vpsDbEntry.contains("tableFiles") && vpsDbEntry["tableFiles"].is_array()) {
                for (const auto& file : vpsDbEntry["tableFiles"]) {
                    if (file.value("tableFormat", "") == "VPX") {
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

            if (!normGameName.empty() && vpsDbEntry.contains("tableFiles") && vpsDbEntry["tableFiles"].is_array()) {
                for (const auto& tableFile : vpsDbEntry["tableFiles"]) {
                    if (tableFile.contains("roms") && tableFile["roms"].is_array()) {
                        for (const auto& rom : tableFile["roms"]) {
                            if (rom.contains("name") && rom["name"].is_string()) {
                                std::string romName = rom["name"].get<std::string>();
                                if (!romName.empty() && utils_.normalizeString(romName) == normGameName) {
                                    currentMatchScore += 5.0f;
                                    LOG_DEBUG("ROM match found for: " << tableData.gameName);
                                    break;
                                }
                            }
                        }
                    }
                }
            }

            float nameSimilarityTableName = 0.0f;
            float nameSimilarityFilename = 0.0f;

            if (!normTableNameAggressive.empty() && normTableNameAggressive == normVpsNameAggressive) {
                nameSimilarityTableName = 3.0f;
                LOG_DEBUG("Exact tableName match: " << tableData.tableName);
            } else if (!normTableNameLessAggressive.empty() && normTableNameLessAggressive == normVpsNameLessAggressive) {
                nameSimilarityTableName = 2.0f;
                LOG_DEBUG("Fuzzy tableName match: " << tableData.tableName);
            } else if (!normTableNameLessAggressive.empty()) {
                size_t dist = levenshteinDistance(normTableNameLessAggressive, normVpsNameLessAggressive);
                float similarity = 1.0f - static_cast<float>(dist) / std::max(normTableNameLessAggressive.size(), normVpsNameLessAggressive.size());
                if (similarity > 0.7f) {
                    nameSimilarityTableName = similarity * 2.0f;
                    LOG_DEBUG("Levenshtein tableName match, similarity=" << similarity);
                }
            }

            if (!normFilename.empty()) {
                if (normFilename == normVpsNameLessAggressive) {
                    nameSimilarityFilename = 3.0f; // Always prioritize filename
                    LOG_DEBUG("Filename match: " << filename << ", score=" << nameSimilarityFilename);
                } else {
                    size_t dist = levenshteinDistance(normFilename, normVpsNameLessAggressive);
                    float similarity = 1.0f - static_cast<float>(dist) / std::max(normFilename.size(), normVpsNameLessAggressive.size());
                    if (similarity > 0.7f) {
                        nameSimilarityFilename = similarity * 3.0f;
                        LOG_DEBUG("Levenshtein filename match, similarity=" << similarity << ", score=" << nameSimilarityFilename);
                    }
                }
            }

            currentMatchScore += std::max(nameSimilarityTableName, nameSimilarityFilename);
            if (nameSimilarityFilename > nameSimilarityTableName) {
                LOG_DEBUG("Filename match better than tableName: filename=" << filename << ", tableName=" << tableData.tableName);
            }

            if (!tableData.year.empty() && !vpsYear.empty() && tableData.year == vpsYear) {
                currentMatchScore += 1.0f;
                LOG_DEBUG("Year match: " << tableData.year);
            }
            if (!tableData.manufacturer.empty() && !vpsManufacturer.empty() &&
                utils_.normalizeStringLessAggressive(tableData.manufacturer) == utils_.normalizeStringLessAggressive(vpsManufacturer)) {
                currentMatchScore += 1.0f;
                LOG_DEBUG("Manufacturer match: " << tableData.manufacturer);
            }

            if (currentMatchScore > bestMatchScore || (currentMatchScore == bestMatchScore && utils_.isVersionGreaterThan(currentVpsEntryLatestVersion, latestVpsVersionFound))) {
                bestMatchScore = currentMatchScore;
                bestVpsDbEntry = vpsDbEntry;
                latestVpsVersionFound = currentVpsEntryLatestVersion;
                bestMatchVpsName = vpsName;
                matched_to_vpsdb = true;
                LOG_DEBUG("New best match, score=" << bestMatchScore << ", vpsName=" << vpsName);
            }
        } catch (const std::exception& e) {
            LOG_DEBUG("Error in VPSDB entry: " << e.what());
            continue;
        }

        processedVpsEntries++;
        if (progress) {
            std::lock_guard<std::mutex> lock(progress->mutex);
            progress->currentTablesLoaded = processedVpsEntries;
        }
    }

    if (matched_to_vpsdb) {
        tableData.vpsId = bestVpsDbEntry.value("id", "");
        tableData.vpsName = bestMatchVpsName;
        tableData.type = bestVpsDbEntry.value("type", "");
        tableData.themes = bestVpsDbEntry.contains("theme") && bestVpsDbEntry["theme"].is_array() ? utils_.join(bestVpsDbEntry["theme"], ", ") : "";
        tableData.designers = bestVpsDbEntry.contains("designers") && bestVpsDbEntry["designers"].is_array() ? utils_.join(bestVpsDbEntry["designers"], ", ") : "";
        tableData.players = bestVpsDbEntry.contains("players") && !bestVpsDbEntry["players"].is_null() && bestVpsDbEntry["players"].is_number_integer() ? std::to_string(bestVpsDbEntry["players"].get<int>()) : "";
        tableData.ipdbUrl = bestVpsDbEntry.value("ipdbUrl", "");

        std::string vpsManufacturer = bestVpsDbEntry.value("manufacturer", "");
        if (!vpsManufacturer.empty() && (tableData.manufacturer.empty() || bestMatchScore > 2.0f)) {
            tableData.manufacturer = vpsManufacturer;
            LOG_DEBUG("Updated manufacturer from VPSDB: " << vpsManufacturer);
        }

        std::string vpsYear;
        if (bestVpsDbEntry.contains("year") && !bestVpsDbEntry["year"].is_null()) {
            vpsYear = bestVpsDbEntry["year"].is_number_integer() ? std::to_string(bestVpsDbEntry["year"].get<int>()) : bestVpsDbEntry["year"].is_string() ? bestVpsDbEntry["year"].get<std::string>() : "";
        }
        if (!vpsYear.empty() && (tableData.year.empty() || bestMatchScore > 2.0f)) {
            tableData.year = vpsYear;
            LOG_DEBUG("Updated year from VPSDB: " << vpsYear);
        }

        if (bestVpsDbEntry.contains("tableFiles") && bestVpsDbEntry["tableFiles"].is_array()) {
            for (const auto& file : bestVpsDbEntry["tableFiles"]) {
                if (file.value("tableFormat", "") == "VPX") {
                    tableData.vpsAuthors = file.contains("authors") && file["authors"].is_array() ? utils_.join(file["authors"], ", ") : "";
                    tableData.features = file.contains("features") && file["features"].is_array() ? utils_.join(file["features"], ", ") : "";
                    std::string current_vps_comment = file.value("comment", "");
                    if (!current_vps_comment.empty() && (tableData.tableDescription.empty() || current_vps_comment.length() > tableData.tableDescription.length())) {
                        tableData.tableDescription = current_vps_comment;
                    }
                    tableData.vpsComment = current_vps_comment;
                }
            }
        }

        float titleSimilarity = 0.0f;
        if (!bestMatchVpsName.empty()) {
            std::string normBestVpsNameLessAggressive = utils_.normalizeStringLessAggressive(bestMatchVpsName);
            std::string sourceName = normTableNameLessAggressive.empty() ? normFilename : normTableNameLessAggressive;
            if (!sourceName.empty() && !normBestVpsNameLessAggressive.empty()) {
                size_t dist = levenshteinDistance(sourceName, normBestVpsNameLessAggressive);
                titleSimilarity = 1.0f - static_cast<float>(dist) / std::max(sourceName.size(), normBestVpsNameLessAggressive.size());
            }
        }
        if (titleSimilarity >= 0.7f && bestMatchScore >= 2.0f) {
            tableData.title = bestMatchVpsName;
            LOG_DEBUG("Title updated to VPSDB name: " << bestMatchVpsName);
        } else {
            tableData.title = tableData.tableName.empty() ? filename : tableData.tableName;
            LOG_DEBUG("Title kept as: " << tableData.title);
        }

        std::string currentVpxVersionNormalized = utils_.normalizeVersion(tableData.tableVersion);
        tableData.vpsVersion = latestVpsVersionFound;
        if (!latestVpsVersionFound.empty() && utils_.isVersionGreaterThan(latestVpsVersionFound, currentVpxVersionNormalized)) {
            if (!currentVpxVersionNormalized.empty()) {
                tableData.tableVersion = currentVpxVersionNormalized + " (Latest: " + latestVpsVersionFound + ")";
            } else {
                tableData.tableVersion = "(Latest: " + latestVpsVersionFound + ")";
            }
            LOG_DEBUG("Updated version: " << tableData.tableVersion);
        } else if (!currentVpxVersionNormalized.empty()) {
            tableData.tableVersion = currentVpxVersionNormalized;
        } else if (!latestVpsVersionFound.empty()) {
            tableData.tableVersion = latestVpsVersionFound;
        }

        tableData.matchConfidence = bestMatchScore / 10.0f;
        LOG_INFO("Matched table to VPSDB, confidence: " << tableData.matchConfidence);
        if (progress) {
            std::lock_guard<std::mutex> lock(progress->mutex);
            progress->numMatched++;
        }
    } else {
        tableData.title = tableData.tableName.empty() ? filename : tableData.tableName;
        {
            std::lock_guard<std::mutex> lock(mismatchLogMutex);
            std::ofstream mismatchLog("tables/vpsdb_mismatches.log", std::ios::app);
            mismatchLog << "No VPSDB match for table: '" << tableData.tableName << "', gameName: '" << tableData.gameName << "', filename: '" << filename << "'\n";
        }
        LOG_INFO("No VPSDB match, using title: " << tableData.title);
        if (progress) {
            std::lock_guard<std::mutex> lock(progress->mutex);
            progress->numNoMatch++;
        }

        if (!filename.empty()) {
            if (tableData.year.empty()) {
                std::regex yearRegex(R"(\b(\d{4})\b)");
                std::smatch match;
                if (std::regex_search(filename, match, yearRegex)) {
                    tableData.year = match[1].str();
                    LOG_DEBUG("Final fallback year: " << tableData.year);
                }
            }
            if (tableData.manufacturer.empty()) {
                std::regex manufRegex(R"(\(([^)]+?)(?:\s+\d{4})?\))");
                std::smatch match;
                if (std::regex_search(filename, match, manufRegex)) {
                    tableData.manufacturer = match[1].str();
                    LOG_DEBUG("Final fallback manufacturer: " << tableData.manufacturer);
                }
            }
        }
    }

    LOG_DEBUG("Final table title: " << tableData.title << ", tableName=" << tableData.tableName << ", manufacturer=" << tableData.manufacturer << ", year=" << tableData.year);
    return matched_to_vpsdb;
}