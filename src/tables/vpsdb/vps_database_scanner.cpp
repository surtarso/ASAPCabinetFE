#include "vps_database_scanner.h"
#include "log/logging.h"
#include <filesystem>
#include <mutex>
#include <fstream>
#include <regex>
#include <set>

namespace fs = std::filesystem;

static std::mutex mismatchLogMutex;

VpsDataScanner::VpsDataScanner(const nlohmann::json& vpsDb) : vpsDb_(vpsDb), utils_() {}

bool VpsDataScanner::matchMetadata(const nlohmann::json& vpxTable, TableData& tableData, LoadingProgress* progress) const {
    if (!vpxTable.is_object()) {
        LOG_DEBUG("VpsDataScanner: vpxTable is not an object, type: " << vpxTable.type_name());
        return false;
    }

    if (tableData.jsonOwner == "Virtual Pinball Spreadsheet Database") {
        LOG_DEBUG("VpsDataScanner: " << tableData.title << " already scanned.");
        return false;
    }

    std::string filename = vpxTable.contains("path") && vpxTable["path"].is_string() ?
                           fs::path(vpxTable["path"].get<std::string>()).stem().string() : "N/A";
    //LOG_INFO("Matching table: path=" << filename << ", title=" << tableData.title << ", tableName=" << tableData.tableName);
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
    // Adjust title based on ROM for ambiguous cases
    std::string romName = tableData.romName;
    std::string normRomName = utils_.normalizeString(romName);
    std::string adjustedTitle = tableData.title;
    std::string originalTitle = tableData.title;
    if (!normRomName.empty()) {
        if (utils_.normalizeStringLessAggressive(tableData.title) == "terminator") {
            if (normRomName == "t2_l8") {
                adjustedTitle = "terminator 2";
                LOG_DEBUG("Adjusted terminator title to terminator 2 due to ROM: romName='" << normRomName << "'");
            } else if (normRomName == "term3") {
                adjustedTitle = "terminator 3";
                LOG_DEBUG("Adjusted terminator title to terminator 3 due to ROM: romName='" << normRomName << "'");
            }
        } else if (utils_.normalizeStringLessAggressive(tableData.title) == "x") {
            if (normRomName == "xfiles") {
                adjustedTitle = "x-files";
                LOG_DEBUG("Adjusted x title to x-files due to ROM: romName='" << normRomName << "'");
            } else if (normRomName == "xmn_151h") {
                adjustedTitle = "x-men";
                LOG_DEBUG("Adjusted x title to x-men due to ROM: romName='" << normRomName << "'");
            }
        } else if (utils_.normalizeStringLessAggressive(tableData.title) == "batman the dark knight" && normRomName == "bdk_294") {
            adjustedTitle = "batman the dark knight";
            LOG_DEBUG("Confirmed batman the dark knight title due to ROM: romName='" << normRomName << "'");
        }
    }
    std::set<std::string> titles;
    std::string filenameTitle = utils_.safeGetString(vpxTable, "filename_title", "");
    // Skip filename_title if it matches original tableData.title to avoid reinforcing bad metadata
    if (!filenameTitle.empty() && filenameTitle != originalTitle) {
        titles.insert(cleanTitle(filenameTitle));
        LOG_DEBUG("Added filename_title: input='" << filenameTitle << "', cleaned='" << cleanTitle(filenameTitle) << "'");
    }
    if (!filename.empty() && filename != "N/A") {
        titles.insert(cleanTitle(filename));
        //LOG_DEBUG("Added filename: input='" << filename << "', cleaned='" << cleanTitle(filename) << "'");
    }
    if (!adjustedTitle.empty() && adjustedTitle != originalTitle) {
        titles.insert(cleanTitle(adjustedTitle));
        LOG_DEBUG("Added adjusted title: input='" << adjustedTitle << "', cleaned='" << cleanTitle(adjustedTitle) << "'");
    }
    if (!tableData.title.empty() && tableData.title != adjustedTitle) {
        titles.insert(cleanTitle(tableData.title));
        LOG_DEBUG("Added tableData.title: input='" << tableData.title << "', cleaned='" << cleanTitle(tableData.title) << "'");
    }
    if (!tableData.tableName.empty()) {
        titles.insert(cleanTitle(tableData.tableName));
        //LOG_DEBUG("Added tableName: input='" << tableData.tableName << "', cleaned='" << cleanTitle(tableData.tableName) << "'");
    }
    std::string manufacturer = utils_.safeGetString(vpxTable, "filename_manufacturer", "");
    if (manufacturer.empty()) manufacturer = tableData.manufacturer.empty() ? tableData.tableManufacturer : tableData.manufacturer;
    std::string year = utils_.safeGetString(vpxTable, "filename_year", "");
    if (year.empty()) year = tableData.year.empty() ? tableData.tableYear : tableData.year;
    if (year.empty()) year = utils_.extractYearFromDate(filename);
    //TODO: add these to settings.h for control on UI
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
                float sim = 1.0f - static_cast<float>(dist) / std::max(normTitle.size(), normVpsName.size());
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
                            LOG_DEBUG("ROM match: romName='" << normRomName << "', score+=" << ROM_WEIGHT);
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
            // This will display each table going thru each entry on vpsdb
            // progress->currentTablesLoaded = processedEntries;
        }
    }
    if (bestScore >= CONFIDENCE_THRESHOLD) {
        tableData.vpsId = utils_.safeGetString(bestMatch, "id", "");
        tableData.vpsName = utils_.safeGetString(bestMatch, "name", "");
        tableData.vpsType = utils_.safeGetString(bestMatch, "type", "");
        tableData.vpsThemes = bestMatch.contains("theme") && bestMatch["theme"].is_array() ?
                              utils_.join(bestMatch["theme"], ", ") : "";
        tableData.vpsDesigners = bestMatch.contains("designers") && bestMatch["designers"].is_array() ?
                                 utils_.join(bestMatch["designers"], ", ") : "";
        tableData.vpsPlayers = bestMatch.contains("players") && bestMatch["players"].is_number_integer() ?
                               std::to_string(bestMatch["players"].get<int>()) : "";
        tableData.vpsIpdbUrl = utils_.safeGetString(bestMatch, "ipdbUrl", "");
        tableData.vpsManufacturer = utils_.safeGetString(bestMatch, "manufacturer", "");
        tableData.vpsYear = bestMatch.contains("year") && bestMatch["year"].is_number_integer() ?
                            std::to_string(bestMatch["year"].get<int>()) : "";
        tableData.matchConfidence = bestScore;
        tableData.jsonOwner = "Virtual Pinball Spreadsheet Database";
        if (tableData.manufacturer.empty()) tableData.manufacturer = tableData.vpsManufacturer;
        if (tableData.year.empty()) tableData.year = tableData.vpsYear;
        if (bestMatch.contains("tableFiles") && bestMatch["tableFiles"].is_array() && !bestMatch["tableFiles"].empty()) {
            const auto& file = bestMatch["tableFiles"][0];
            tableData.vpsFormat = utils_.safeGetString(file, "tableFormat", "");
            tableData.vpsTableImgUrl = utils_.safeGetString(file, "imgUrl", "");
            tableData.vpsTableUrl = file.contains("urls") && file["urls"].is_array() && !file["urls"].empty() ?
                                    utils_.safeGetString(file["urls"][0], "url", "") : "";
            tableData.vpsAuthors = file.contains("authors") && file["authors"].is_array() ?
                                   utils_.join(file["authors"], ", ") : "";
            tableData.vpsFeatures = file.contains("features") && file["features"].is_array() ?
                                    utils_.join(file["features"], ", ") : "";
            tableData.vpsComment = utils_.safeGetString(file, "comment", "");
            tableData.vpsVersion = bestVpsVersion;
        }
        if (bestMatch.contains("b2sFiles") && bestMatch["b2sFiles"].is_array() && !bestMatch["b2sFiles"].empty()) {
            const auto& file = bestMatch["b2sFiles"][0];
            tableData.vpsB2SImgUrl = utils_.safeGetString(file, "imgUrl", "");
            tableData.vpsB2SUrl = file.contains("urls") && file["urls"].is_array() && !file["urls"].empty() ?
                                  utils_.safeGetString(file["urls"][0], "url", "") : "";
        }
        std::string currentVersion = utils_.normalizeVersion(tableData.tableVersion);
        if (!bestVpsVersion.empty() && utils_.isVersionGreaterThan(bestVpsVersion, currentVersion)) {
            tableData.tableVersion = currentVersion.empty() ? bestVpsVersion :
                                     currentVersion + " (Latest: " + bestVpsVersion + ")";
        }
        LOG_INFO("Matched table: " << tableData.vpsName << ", confidence: " << bestScore);
        if (progress) {
            std::lock_guard<std::mutex> lock(progress->mutex);
            progress->numMatched++;
            progress->logMessages.push_back("Matched " + filename + " to " + tableData.vpsName + ", score: " + std::to_string(bestScore));
        }
        return true;
    } else {
        {
            std::lock_guard<std::mutex> lock(mismatchLogMutex);
            std::ofstream mismatchLog("logs/vpsdb_mismatches.log", std::ios::app);
            mismatchLog << "No match for: title='" << tableData.title << "', tableName='" << tableData.tableName
                        << "', romName='" << romName << "', filename='" << filename
                        << "', year='" << year << "', manufacturer='" << manufacturer
                        << "', score=" << bestScore;
            if (bestScore >= 0.3) {
                mismatchLog << ", near_match='" << bestVpsName << "'";
            }
            mismatchLog << "\n";
        }
        LOG_WARN("No VPSDB match for: " << filename << ", best score: " << bestScore);
        if (progress) {
            std::lock_guard<std::mutex> lock(progress->mutex);
            progress->numNoMatch++;
            progress->logMessages.push_back("No match for " + filename + ", score: " + std::to_string(bestScore));
        }
        return false;
    }
}