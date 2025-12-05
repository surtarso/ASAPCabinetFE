#include "vps_database_scanner.h"
#include "log/logging.h"
#include <filesystem>
#include <mutex>
#include <fstream>
#include <regex>
#include <set>

namespace fs = std::filesystem;

static std::mutex mismatchLogMutex;

VpsDataScanner::VpsDataScanner(const nlohmann::json& vpsDb, const Settings& settings) : vpsDb_(vpsDb), utils_(), settings_(settings) {}

// Private helper
void VpsDataScanner::populateFromVpsEntry(const nlohmann::json& entry, TableData& tableData, float confidence) const {
    tableData.vpsId = utils_.safeGetString(entry, "id", "");
    tableData.vpsName = utils_.safeGetString(entry, "name", "");
    tableData.vpsType = utils_.safeGetString(entry, "type", "");
    tableData.vpsThemes = entry.contains("theme") && entry["theme"].is_array() ?
                          utils_.join(entry["theme"], ", ") : "";
    tableData.vpsDesigners = entry.contains("designers") && entry["designers"].is_array() ?
                             utils_.join(entry["designers"], ", ") : "";
    tableData.vpsPlayers = entry.contains("players") && entry["players"].is_number_integer() ?
                           std::to_string(entry["players"].get<int>()) : "";
    tableData.vpsIpdbUrl = utils_.safeGetString(entry, "ipdbUrl", "");
    tableData.vpsManufacturer = utils_.safeGetString(entry, "manufacturer", "");
    tableData.vpsYear = entry.contains("year") && entry["year"].is_number_integer() ?
                        std::to_string(entry["year"].get<int>()) : "";
    tableData.matchConfidence = confidence;
    tableData.jsonOwner = "Virtual Pinball Spreadsheet Database";

    if (tableData.bestManufacturer.empty()) tableData.bestManufacturer = tableData.vpsManufacturer;
    if (tableData.bestYear.empty()) tableData.bestYear = tableData.vpsYear;

    std::string vpsVersion;
    if (entry.contains("tableFiles") && entry["tableFiles"].is_array() && !entry["tableFiles"].empty()) {
        for (const auto& file : entry["tableFiles"]) {
            if (utils_.safeGetString(file, "tableFormat", "") == "VPX") {
                std::string version = utils_.safeGetString(file, "version", "");
                if (utils_.isVersionGreaterThan(version, vpsVersion)) {
                    vpsVersion = version;
                }
            }
        }
        const auto& file = entry["tableFiles"][0];  // Use first as before
        tableData.vpsFormat = utils_.safeGetString(file, "tableFormat", "");
        tableData.vpsTableImgUrl = utils_.safeGetString(file, "imgUrl", "");
        tableData.vpsTableUrl = file.contains("urls") && file["urls"].is_array() && !file["urls"].empty() ?
                                utils_.safeGetString(file["urls"][0], "url", "") : "";
        tableData.vpsAuthors = file.contains("authors") && file["authors"].is_array() ?
                               utils_.join(file["authors"], ", ") : "";
        tableData.vpsFeatures = file.contains("features") && file["features"].is_array() ?
                                utils_.join(file["features"], ", ") : "";
        tableData.vpsComment = utils_.safeGetString(file, "comment", "");
        tableData.vpsVersion = vpsVersion;
    }
    if (entry.contains("b2sFiles") && entry["b2sFiles"].is_array() && !entry["b2sFiles"].empty()) {
        const auto& file = entry["b2sFiles"][0];
        tableData.vpsB2SImgUrl = utils_.safeGetString(file, "imgUrl", "");
        tableData.vpsB2SUrl = file.contains("urls") && file["urls"].is_array() && !file["urls"].empty() ?
                              utils_.safeGetString(file["urls"][0], "url", "") : "";
    }

    // Version tag logic (same as before)
    std::string currentVersion = utils_.normalizeVersion(tableData.tableVersion);
    std::string vpsNormVersion = utils_.normalizeVersion(vpsVersion);
    if (!vpsNormVersion.empty()) {
        if (utils_.isVersionGreaterThan(vpsNormVersion, currentVersion)) {
            tableData.bestVersion = currentVersion.empty() ? vpsNormVersion : currentVersion + " (Behind: " + vpsNormVersion + ")";
        } else if (utils_.isVersionGreaterThan(currentVersion, vpsNormVersion)) {
            tableData.bestVersion = currentVersion + " (Ahead: " + vpsNormVersion + ")";
        } else {
            tableData.bestVersion = currentVersion;
        }
    } else {
        tableData.bestVersion = currentVersion;
    }
}

bool VpsDataScanner::matchMetadata(const nlohmann::json& vpxTable, TableData& tableData, LoadingProgress* progress) const {
    if (!vpxTable.is_object()) {
        LOG_DEBUG("vpxTable is not an object, type: " + std::string(vpxTable.type_name()));
        return false;
    }

    std::string filename = vpxTable.contains("path") && vpxTable["path"].is_string() ?
                           fs::path(vpxTable["path"].get<std::string>()).stem().string() : "N/A";
    //LOG_INFO("Matching table: path=" << filename << ", title=" << tableData.bestTitle << ", tableName=" << tableData.tableName);

    // collect File Metadata information
    if (vpxTable.contains("table_info") && vpxTable["table_info"].is_object()) {
        const auto& tableInfo = vpxTable["table_info"];
        tableData.tableName = utils_.cleanString(utils_.safeGetString(tableInfo, "table_name", ""));
        tableData.tableAuthor = utils_.cleanString(utils_.safeGetString(tableInfo, "author_name", ""));
        tableData.tableDescription = utils_.cleanString(utils_.safeGetString(tableInfo, "table_description", ""));
        tableData.tableSaveDate = utils_.safeGetString(tableInfo, "table_save_date", "");
        tableData.tableReleaseDate = utils_.safeGetString(tableInfo, "release_date", "");
        tableData.tableVersion = utils_.cleanString(utils_.safeGetString(tableInfo, "table_version", ""));
        tableData.tableRevision = utils_.cleanString(utils_.safeGetString(tableInfo, "table_save_rev", ""));
        tableData.tableBlurb = utils_.cleanString(utils_.safeGetString(tableInfo, "table_blurb", ""));
        tableData.tableRules = utils_.cleanString(utils_.safeGetString(tableInfo, "table_rules", ""));
        tableData.tableAuthorEmail = utils_.cleanString(utils_.safeGetString(tableInfo, "author_email", ""));
        tableData.tableAuthorWebsite = utils_.cleanString(utils_.safeGetString(tableInfo, "author_website", ""));
    }
    if (vpxTable.contains("properties") && vpxTable["properties"].is_object()) {
        const auto& properties = vpxTable["properties"];
        tableData.tableType = utils_.cleanString(utils_.safeGetString(properties, "TableType", ""));
        tableData.tableManufacturer = utils_.cleanString(utils_.safeGetString(properties, "CompanyName",
                                                     utils_.safeGetString(properties, "Company", "")));
        tableData.tableYear = utils_.cleanString(utils_.safeGetString(properties, "CompanyYear",
                                                     utils_.safeGetString(properties, "Year", "")));
    }
    if (tableData.romName.empty()) {
        tableData.romName = utils_.safeGetString(vpxTable, "rom", "");
    }

    // If vpsId exists and not forcing rebuild, do direct lookup
    if (!tableData.vpsId.empty() && !settings_.forceRebuildMetadata) {
        for (const auto& entry : vpsDb_) {
            if (utils_.safeGetString(entry, "id", "") == tableData.vpsId) {
                populateFromVpsEntry(entry, tableData, 1.0f);  // Full confidence
                LOG_INFO("Direct ID match for " + filename + ": " + tableData.vpsName);
                if (progress) {
                    std::lock_guard<std::mutex> lock(progress->mutex);
                    progress->numMatched++;
                    progress->logMessages.push_back("Direct ID match for " + filename + " to " + tableData.vpsName);
                }
                return true;
            }
        }
        // If ID not found, log and fallback to matchmaking
        LOG_WARN("VPSDB ID " + tableData.vpsId + " not found for " + filename + ", falling back to matchmaking");
    }

    // Early skip check (moved after direct lookup)
    if (tableData.jsonOwner == "Virtual Pinball Spreadsheet Database" && !settings_.forceRebuildMetadata) {
        LOG_DEBUG(tableData.bestTitle + " already scanned.");
        return false;
    }

    // add suffixes to a list, re-use them later for better matching on variants?
    auto cleanTitle = [&](const std::string& input) -> std::string {
        if (input.empty()) return "";
        std::string cleaned = input;
        std::regex suffixes(R"(\s*(?:Chrome Edition|Sinister Six Edition|1920 Mod|Premium|Pro|LE|Never Say Die|Power Up Edition|Classic|Pinball Wizard|Quest for Money|-.*$|\(.*\)|:.*$|JP's\s*|HH Mod\s*))", std::regex::icase);
        cleaned = std::regex_replace(cleaned, suffixes, "");
        cleaned = utils_.cleanString(cleaned);
        std::regex articles(R"(\b(The|A|An)\b\s*)", std::regex::icase);
        cleaned = std::regex_replace(cleaned, articles, "");
        return utils_.extractCleanTitle(cleaned);
    };

    // Adjust title based on ROM for ambiguous cases (create a helper with a list!)
    std::string romName = tableData.romName;
    std::string normRomName = utils_.normalizeString(romName);
    std::string adjustedTitle = tableData.bestTitle;
    std::string originalTitle = tableData.bestTitle;
    if (!normRomName.empty()) {
        if (utils_.normalizeStringLessAggressive(tableData.bestTitle) == "terminator") {
            if (normRomName == "t2_l8") {
                adjustedTitle = "terminator 2";
                LOG_DEBUG("Adjusted terminator title to terminator 2 due to ROM: romName='" + normRomName + "'");
            } else if (normRomName == "term3") {
                adjustedTitle = "terminator 3";
                LOG_DEBUG("Adjusted terminator title to terminator 3 due to ROM: romName='" + normRomName + "'");
            }
        } else if (utils_.normalizeStringLessAggressive(tableData.bestTitle) == "x") {
            if (normRomName == "xfiles") {
                adjustedTitle = "x-files";
                LOG_DEBUG("Adjusted x title to x-files due to ROM: romName='" + normRomName + "'");
            } else if (normRomName == "xmn_151h") {
                adjustedTitle = "x-men";
                LOG_DEBUG("Adjusted x title to x-men due to ROM: romName='" + normRomName + "'");
            }
        } else if (utils_.normalizeStringLessAggressive(tableData.bestTitle) == "batman the dark knight" && normRomName == "bdk_294") {
            adjustedTitle = "batman the dark knight";
            LOG_DEBUG("Confirmed batman the dark knight title due to ROM: romName='" + normRomName + "'");
        }
    }

    // Title (using all available)
    std::set<std::string> titles;
    std::string filenameTitle = utils_.safeGetString(vpxTable, "filename_title", "");
    // Skip filename_title if it matches original tableData.bestTitle to avoid reinforcing bad metadata
    if (!filenameTitle.empty() && filenameTitle != originalTitle) {
        titles.insert(cleanTitle(filenameTitle));
        LOG_DEBUG("Added filename_title: input='" + filenameTitle + "', cleaned='" + cleanTitle(filenameTitle) + "'");
    }
    if (!filename.empty() && filename != "N/A") {
        titles.insert(cleanTitle(filename));
        //LOG_DEBUG("Added filename: input='" + filename + "', cleaned='" + cleanTitle(filename) << "'");
    }
    if (!adjustedTitle.empty() && adjustedTitle != originalTitle) {
        titles.insert(cleanTitle(adjustedTitle));
        LOG_DEBUG("Added adjusted title: input='" + adjustedTitle + "', cleaned='" + cleanTitle(adjustedTitle) + "'");
    }
    if (!tableData.bestTitle.empty() && tableData.bestTitle != adjustedTitle) {
        titles.insert(cleanTitle(tableData.bestTitle));
        LOG_DEBUG("Added tableData.bestTitle: input='" + tableData.bestTitle + "', cleaned='" + cleanTitle(tableData.bestTitle) + "'");
    }
    if (!tableData.tableName.empty()) {
        titles.insert(cleanTitle(tableData.tableName));
        //LOG_DEBUG("Added tableName: input='" + tableData.tableName + "', cleaned='" + cleanTitle(tableData.tableName) << "'");
    }

    // Manufacturer
    std::string manufacturer = utils_.safeGetString(vpxTable, "filename_manufacturer", "");
    if (manufacturer.empty()) manufacturer = tableData.bestManufacturer.empty() ? tableData.tableManufacturer : tableData.bestManufacturer;

    // Year
    std::string year = utils_.safeGetString(vpxTable, "filename_year", "");
    if (year.empty()) year = tableData.bestYear.empty() ? tableData.tableYear : tableData.bestYear;
    // if (year.empty()) year = utils_.extractYearFromDate(filename); // again?

    // Weight control
    const float TITLE_WEIGHT = settings_.titleWeight;// 0.6f;
    const float YEAR_WEIGHT = settings_.yearWeight;// 0.2f;
    const float MANUFACTURER_WEIGHT = settings_.manufacturerWeight;// 0.1f;
    const float ROM_WEIGHT = settings_.romWeight;// 0.25f;
    const float LEVENSHTEIN_THRESHOLD = settings_.titleThreshold;// 0.55f;
    const float CONFIDENCE_THRESHOLD = settings_.confidenceThreshold;// 0.6f;
    float bestScore = 0.0f;

    nlohmann::json bestMatch;
    std::string bestVpsVersion;
    std::string bestVpsName;
    size_t processedEntries = 0;
    if (progress) {
        std::lock_guard<std::mutex> lock(progress->mutex);
        progress->currentTask = "Matching VPSDB " + std::to_string(vpsDb_.size()) + " entries...";
        // progress->currentTablesLoaded = 0; // This is relative to entries on vpsdb
    }

    // ============================================== Start Matching
    for (const auto& entry : vpsDb_) {
        if (!entry.is_object() || !entry.contains("name") || entry["name"].is_null()) {
            continue;
        }
        std::string vpsName = utils_.safeGetString(entry, "name", "");
        std::string normVpsName = utils_.normalizeStringLessAggressive(vpsName);
        std::string vpsManufacturer = utils_.safeGetString(entry, "manufacturer", "");
        std::string normVpsManufacturer = utils_.normalizeStringLessAggressive(vpsManufacturer);
        std::string vpsYear = entry.contains("year") && entry["year"].is_number_integer() ?
                              std::to_string(entry["year"].get<int>()) : "";
        float score = 0.0f;
        float bestTitleScore = 0.0f;
        for (const auto& title : titles) {
            std::string normTitle = utils_.normalizeStringLessAggressive(title);
            if (normTitle.empty()) continue;
            if (utils_.toLower(normTitle) == utils_.toLower(normVpsName)) {
                bestTitleScore = std::max(bestTitleScore, TITLE_WEIGHT);
                //LOG_DEBUG("Exact title match (case-insensitive): normTitle='" << normTitle << "', normVpsName='" << normVpsName << "', score=" << TITLE_WEIGHT);
            } else {
                size_t dist = utils_.levenshteinDistance(normTitle, normVpsName);
                float denom = static_cast<float>(std::max(normTitle.size(), normVpsName.size()));
                float sim = 1.0f - static_cast<float>(dist) / denom;
                if (sim >= LEVENSHTEIN_THRESHOLD) {
                    bestTitleScore = std::max(bestTitleScore, sim * TITLE_WEIGHT);
                    //LOG_DEBUG("Levenshtein match: normTitle='" << normTitle << "', normVpsName='" << normVpsName << "', dist=" << dist << ", sim=" << sim << ", score=" << sim * TITLE_WEIGHT);
                }
            }
        }
        score += bestTitleScore;
        if (!year.empty() && year == vpsYear) {
            score += YEAR_WEIGHT;
            //LOG_DEBUG("Year match: year='" << year << "', vpsYear='" << vpsYear << "', score+=" << YEAR_WEIGHT);
        }
        if (!manufacturer.empty() && normVpsManufacturer == utils_.normalizeStringLessAggressive(manufacturer)) {
            score += MANUFACTURER_WEIGHT;
            //LOG_DEBUG("Manufacturer match: manufacturer='" << manufacturer << "', normVpsManufacturer='" << normVpsManufacturer << "', score+=" << MANUFACTURER_WEIGHT);
        }
        if (!normRomName.empty() && entry.contains("tableFiles") && entry["tableFiles"].is_array()) {
            for (const auto& file : entry["tableFiles"]) {
                if (file.contains("roms") && file["roms"].is_array()) {
                    for (const auto& rom : file["roms"]) {
                        if (rom.contains("name") && utils_.normalizeString(utils_.safeGetString(rom, "name", "")) == normRomName) {
                            score += ROM_WEIGHT;
                            LOG_DEBUG("ROM match: romName='" + normRomName + "', score+=" + std::to_string(ROM_WEIGHT));
                            break;
                        }
                    }
                }
                if (score >= ROM_WEIGHT) break;
            }
        }
        std::string vpsVersion;
        if (entry.contains("tableFiles") && entry["tableFiles"].is_array()) {
            for (const auto& file : entry["tableFiles"]) {
                if (utils_.safeGetString(file, "tableFormat", "") == "VPX") {
                    std::string version = utils_.safeGetString(file, "version", "");
                    if (utils_.isVersionGreaterThan(version, vpsVersion)) {
                        vpsVersion = version;
                    }
                }
            }
        }
        if (score > bestScore) {
            bestScore = score;
            bestMatch = entry;
            bestVpsVersion = vpsVersion;
            bestVpsName = vpsName;
        }
        processedEntries++;
        if (progress) {
            std::lock_guard<std::mutex> lock(progress->mutex);
            // This will display each table going thru each entry on vpsdb (unwanted)
            // progress->currentTablesLoaded = processedEntries;
        }
    }

    // ============================================== MATCH!
    if (bestScore >= CONFIDENCE_THRESHOLD) {

        populateFromVpsEntry(bestMatch, tableData, bestScore);

        // --- Done, count table and move on ---
        LOG_INFO("Matched table: " + tableData.vpsName + ", confidence: " + std::to_string(bestScore));
        if (progress) {
            std::lock_guard<std::mutex> lock(progress->mutex);
            progress->numMatched++;
            progress->logMessages.push_back("Matched " + filename + " to " + tableData.vpsName + ", score: " + std::to_string(bestScore));
        }
        return true;

    // ============================================== NO MATCH!
    } else {
        {
            std::lock_guard<std::mutex> lock(mismatchLogMutex);
            // std::ofstream mismatchLog("logs/vpsdb_mismatches.log", std::ios::app);
            fs::path logPath(settings_.vpsdbMissmatchLog);
            fs::create_directories(logPath.parent_path());
            // LOG_DEBUG("VPSDB Missmatch logPath: " + logPath.string()); // why not resolved?!
            std::ofstream mismatchLog(settings_.vpsdbMissmatchLog, std::ios::app);
            // LOG_DEBUG("VPSDB Missmatch log file: " + settings_.vpsdbMissmatchLog);  // not resolved?

            mismatchLog << "No match for: title='" << tableData.bestTitle << "', tableName='" << tableData.tableName
                        << "', romName='" << romName << "', filename='" << filename
                        << "', year='" << year << "', manufacturer='" << manufacturer
                        << "', score=" << bestScore;
            if (bestScore >= 0.3) {
                mismatchLog << ", near_match='" << bestVpsName << "'";
            }
            mismatchLog << "\n";
        }

        // --- Done, count table and move on ---
        LOG_WARN("No VPSDB match for: " + filename + ", best score: " + std::to_string(bestScore));
        if (progress) {
            std::lock_guard<std::mutex> lock(progress->mutex);
            progress->numNoMatch++;
            progress->logMessages.push_back("No match for " + filename + ", score: " + std::to_string(bestScore));
        }
        return false;
    }
}
