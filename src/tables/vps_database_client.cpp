#include "vps_database_client.h"
#include <fstream>
#include <regex>
#include <algorithm>
#include <cctype>
#include <curl/curl.h>
#include <filesystem>
#include <json.hpp>
#include "utils/logging.h"
#include "table_loader.h" // Include TableLoader.h to get TableData definition

namespace fs = std::filesystem;

// Callback for curl to write received data
static size_t writeCallback(void* contents, size_t size, size_t nmemb, std::string* userp) {
    size_t realsize = size * nmemb;
    userp->append((char*)contents, realsize);
    return realsize;
}

// Callback for curl to capture headers
static size_t headerCallback(char* buffer, size_t size, size_t nitems, std::string* userp) {
    size_t realsize = size * nitems;
    userp->append(buffer, realsize);
    return realsize;
}

// Joins a JSON array into a delimited string
static std::string join(const nlohmann::json& array, const std::string& delimiter) {
    std::vector<std::string> items;
    for (const auto& item : array) {
        try {
            if (item.is_string()) { // Added check for string type
                items.push_back(item.get<std::string>());
            }
        } catch (const std::exception& e) {
            LOG_DEBUG("VpsDatabaseClient: Skipping invalid array item in join: " << e.what());
        }
    }
    return items.empty() ? "" : std::accumulate(std::next(items.begin()), items.end(), items[0],
        [&delimiter](const std::string& a, const std::string& b) { return a + delimiter + b; });
}

VpsDatabaseClient::VpsDatabaseClient(const std::string& vpsDbPath) : vpsDbPath_(vpsDbPath) {}

bool VpsDatabaseClient::load() {
    try {
        std::ifstream file(vpsDbPath_);
        if (!file.is_open()) {
            LOG_ERROR("VpsDatabaseClient: Failed to open vpsdb.json at: " << vpsDbPath_);
            return false;
        }
        vpsDb_ = nlohmann::json::parse(file);
        if (vpsDb_.is_array()) {
            // Root is an array of tables
            LOG_INFO("VpsDatabaseClient: Loaded vpsdb.json with " << vpsDb_.size() << " entries");
            return true;
        } else if (vpsDb_.is_object() && vpsDb_.contains("tables") && vpsDb_["tables"].is_array()) {
            // Root is an object with a "tables" array; reassign to the array
            vpsDb_ = vpsDb_["tables"];
            LOG_INFO("VpsDatabaseClient: Loaded vpsdb.json with " << vpsDb_.size() << " entries");
            return true;
        } else {
            LOG_ERROR("VpsDatabaseClient: Invalid vpsdb.json: expected an array or an object with 'tables' array");
            return false;
        }
    } catch (const std::exception& e) {
        LOG_ERROR("VpsDatabaseClient: Failed to parse vpsdb.json: " << e.what());
        return false;
    }
}

// Aggressive normalization (removes all non-alphanumeric, lowercases)
std::string VpsDatabaseClient::normalizeString(const std::string& input) const {
    std::string result = input;
    std::transform(result.begin(), result.end(), result.begin(), ::tolower);
    result.erase(std::remove_if(result.begin(), result.end(), [](char c) {
        return !(std::isalnum(c)); // Keep only alphanumeric characters
    }), result.end());
    return result;
}

// Less aggressive normalization (keeps spaces, lowercases, removes some punctuation)
std::string VpsDatabaseClient::normalizeStringLessAggressive(const std::string& input) const {
    std::string result = input;
    std::transform(result.begin(), result.end(), result.begin(), ::tolower);
    // Remove common punctuation and symbols, but preserve spaces (and alphanumeric chars)
    result.erase(std::remove_if(result.begin(), result.end(), [](char c) {
        return (c == '_' || c == '-' || c == '(' || c == ')' || c == '.' || c == '\'' || c == ',' || c == '!' || c == '?' || c == ':' || c == '&' || c == '[' || c == ']');
    }), result.end());

    // Collapse multiple spaces into one
    std::string cleaned_result;
    std::unique_copy(result.begin(), result.end(), std::back_inserter(cleaned_result),
                     [](char a, char b) { return std::isspace(a) && std::isspace(b); });

    // Trim leading/trailing spaces
    size_t first = cleaned_result.find_first_not_of(' ');
    if (std::string::npos == first) return ""; // String was all spaces
    size_t last = cleaned_result.find_last_not_of(' ');
    return cleaned_result.substr(first, (last - first + 1));
}

// Helper to normalize version strings (replace comma with dot, remove leading/trailing spaces)
// Also handles "X.X-Y.Y" and extracts the first part.
static std::string normalizeVersion(const std::string& version) {
    std::string normalized = version;
    std::replace(normalized.begin(), normalized.end(), ',', '.');
    // Remove leading/trailing spaces
    normalized.erase(0, normalized.find_first_not_of(" \t\n\r\f\v"));
    normalized.erase(normalized.find_last_not_of(" \t\n\r\f\v") + 1);

    // If version contains "X.X-Y.Y" (e.g., "3.0-3.0" or "2.1.0-2.6"), try to get the first part.
    size_t dash_pos = normalized.find('-');
    if (dash_pos != std::string::npos) {
        std::string first_part = normalized.substr(0, dash_pos);
        // Check if the first part looks like a version number (contains digits and possibly dots)
        if (std::regex_match(first_part, std::regex("[0-9\\.]+"))) {
            return first_part;
        }
    }
    return normalized;
}

// Helper to compare version strings (simple numeric comparison if possible, else string)
// Returns true if v1 is numerically greater than v2
static bool isVersionGreaterThan(const std::string& v1, const std::string& v2) {
    std::string norm_v1 = normalizeVersion(v1);
    std::string norm_v2 = normalizeVersion(v2);

    if (norm_v1.empty()) return false; // An empty version is not greater than anything
    if (norm_v2.empty()) return true;  // Anything is greater than an empty version

    // Attempt to convert to floats for numeric comparison
    try {
        float f1 = std::stof(norm_v1);
        float f2 = std::stof(norm_v2);
        return f1 > f2;
    } catch (const std::exception&) {
        // Fallback to string comparison if numeric conversion fails (e.g., "1.4b" vs "1.4")
        return norm_v1 > norm_v2;
    }
}

// Helper to extract year from date string (e.g., "DD.MM.YYYY" or "YYYY")
std::string VpsDatabaseClient::extractYearFromDate(const std::string& dateString) const {
    std::regex dateRegex_DDMMYYYY("\\d{2}\\.\\d{2}\\.(\\d{4})"); // Matches DD.MM.YYYY
    std::regex dateRegex_YYYY("\\d{4}"); // Matches 4-digit year
    std::smatch match;

    if (std::regex_search(dateString, match, dateRegex_DDMMYYYY) && match.size() > 1) {
        return match[1].str();
    } else if (std::regex_search(dateString, match, dateRegex_YYYY) && match.size() > 0) {
        return match[0].str();
    }
    return "";
}

bool VpsDatabaseClient::enrichTableData(const nlohmann::json& vpxTable, TableData& tableData) const {
    if (!vpxTable.is_object()) {
        LOG_DEBUG("VpsDatabaseClient: vpxTable is not an object, type: " << vpxTable.type_name());
        return false;
    }

    // --- Phase 1: Extract and populate initial tableData from vpxTable (vpxtool_index.json) ---
    // These variables will hold the *source* data from vpxtool, which might be incomplete or inaccurate.
    // tableData members (e.g., tableData.tableName, tableData.romPath) will be updated as we proceed.
    std::string vpxTableName_source = ""; // From vpxtool's table_info.table_name
    std::string vpxGameName_source = "";  // From vpxtool's game_name (ROM name)
    std::string vpxTableVersion_source = ""; // From vpxtool's table_info.table_version
    std::string vpxReleaseDate_source = ""; // From vpxtool's table_info.release_date
    std::string vpxAuthorName_source = ""; // From vpxtool's table_info.author_name
    std::string vpxTableDescription_source = ""; // From vpxtool's table_info.table_description

    try {
        // Get VPX table name (from table_info or fallback to filename stem)
        if (vpxTable.contains("table_info") && vpxTable["table_info"].is_object()) {
            const auto& tableInfo = vpxTable["table_info"];
            if (tableInfo.contains("table_name") && tableInfo["table_name"].is_string()) {
                vpxTableName_source = tableInfo["table_name"].get<std::string>();
            }
            // Robustly get table_version
            if (tableInfo.contains("table_version")) {
                if (tableInfo["table_version"].is_string()) {
                    vpxTableVersion_source = tableInfo["table_version"].get<std::string>();
                } else if (tableInfo["table_version"].is_number()) {
                    vpxTableVersion_source = std::to_string(tableInfo["table_version"].get<double>());
                }
            }
            if (tableInfo.contains("release_date") && tableInfo["release_date"].is_string()) {
                vpxReleaseDate_source = tableInfo["release_date"].get<std::string>();
            }
            if (tableInfo.contains("author_name") && tableInfo["author_name"].is_string()) {
                vpxAuthorName_source = tableInfo["author_name"].get<std::string>();
            }
            if (tableInfo.contains("table_description") && tableInfo["table_description"].is_string()) {
                vpxTableDescription_source = tableInfo["table_description"].get<std::string>();
            }
        }

        // Get ROM name (game_name)
        if (vpxTable.contains("game_name") && vpxTable["game_name"].is_string()) {
            vpxGameName_source = vpxTable["game_name"].get<std::string>();
        }

        // Initialize tableData fields.
        // tableData.title is ALREADY set by TableLoader from filename stem before this function.
        // We set tableData.tableName from vpxtool's metadata, if available.
        if (!vpxTableName_source.empty()) {
            tableData.tableName = vpxTableName_source;
        }
        // If vpxTableName_source was empty, tableData.tableName retains its value from TableLoader (filename stem)
        // Ensure tableData.tableName is never empty, fallback to title (filename stem) if necessary
        if (tableData.tableName.empty()) {
            tableData.tableName = tableData.title; // TableData.title is initialized from filename stem
        }

        tableData.tableVersion = vpxTableVersion_source;
        tableData.authorName = vpxAuthorName_source;
        tableData.tableDescription = vpxTableDescription_source;
        tableData.romPath = vpxGameName_source; // Store ROM name in romPath
        tableData.gameName = vpxGameName_source; // Also in gameName for consistency

        // Extract and set year/manufacturer using a robust priority:
        // 1. From release_date
        // 2. From table_name using regex (e.g., "(Manufacturer Year)")
        // 3. From filename (already done by TableLoader before enrichment)

        // Try to get year from release_date first
        if (!vpxReleaseDate_source.empty()) {
            tableData.year = extractYearFromDate(vpxReleaseDate_source);
        }

        // If year is still empty, try to extract from tableData.tableName (which could be the vpxtool table_name or the filename stem)
        if (tableData.year.empty() && !tableData.tableName.empty()) {
            std::regex yearFromTitleRegex("\\((\\d{4})\\)");
            std::smatch match;
            if (std::regex_search(tableData.tableName, match, yearFromTitleRegex)) {
                tableData.year = match[1].str();
            }
        }
        // Manufacturer extraction from tableData.tableName (which could be the vpxtool table_name or the filename stem)
        if (tableData.manufacturer.empty() && !tableData.tableName.empty()) {
            // Regex for " (Manufacturer)" or " (Manufacturer Year)" in table name
            std::regex manufFromTitleRegex("\\(([^)]+?)(?:\\s+\\d{4})?\\)");
            std::smatch match;
            if (std::regex_search(tableData.tableName, match, manufFromTitleRegex) && match.size() > 1) {
                tableData.manufacturer = match[1].str();
            }
        }

    } catch (const nlohmann::json::exception& e) {
        LOG_ERROR("VpsDatabaseClient: JSON parsing error during initial vpxTable field extraction for path: " << vpxTable.value("path", "N/A") << ". Error: " << e.what());
        // This is where the type_error.302 likely originates. We log it and continue.
        // It's important to not throw here to allow processing of other tables.
        // The problematic fields will just remain empty or their default value.
    } catch (const std::exception& e) {
        LOG_ERROR("VpsDatabaseClient: General error during initial vpxTable field extraction for path: " << vpxTable.value("path", "N/A") << ". Error: " << e.what());
        return false; // For other critical errors, we might want to stop processing this table.
    }

    std::ofstream mismatchLog("data/vpsdb_mismatches.log", std::ios::app);
    bool matched_to_vpsdb_entry = false; // Flag to track if ANY VPSDB entry was matched

    std::string latestVpsVersionFound = "";
    nlohmann::json bestVpsDbEntry = nlohmann::json::object();
    
    // Keep track of the VPS name from the best match, for setting tableData.title
    std::string bestMatchVpsName = "";

    // Normalize VPX table data for matching
    std::string normVpxTableNameAggressive = normalizeString(tableData.tableName);
    std::string normVpxTableNameLessAggressive = normalizeStringLessAggressive(tableData.tableName);
    std::string normVpxGameName = normalizeString(tableData.romPath); // ROM name is often aggressive

    LOG_DEBUG("VpsDatabaseClient: Attempting to enrich VPX table '" << tableData.tableName << "' (Normalized: '" << normVpxTableNameAggressive << "', Less Aggressive: '" << normVpxTableNameLessAggressive << "') with Game Name '" << tableData.romPath << "' (Normalized: '" << normVpxGameName << "') Year: '" << tableData.year << "', Manufacturer: '" << tableData.manufacturer << "'. VPX Current Version: '" << tableData.tableVersion << "'");

    // Scoring for different match types (higher means better match)
    // -1: no match
    // 0: Year and Manufacturer match (weakest, fallback, only if no name/ROM match)
    // 1: ROM name match
    // 2: Less aggressive table name match
    // 3: Aggressive table name match (best)
    int bestMatchScore = -1;

    for (const auto& vpsDbEntry : vpsDb_) {
        std::string vpsId, vpsName, vpsYear, vpsManufacturer;
        std::string currentVpsEntryLatestVersion = "";

        try {
            vpsId = vpsDbEntry.value("id", "N/A_ID");
            vpsName = vpsDbEntry.value("name", "");
            
            // Robustly get year from VPSDB
            if (vpsDbEntry.contains("year")) {
                if (vpsDbEntry["year"].is_number_integer()) {
                    vpsYear = std::to_string(vpsDbEntry["year"].get<int>());
                } else if (vpsDbEntry["year"].is_string()) {
                    vpsYear = vpsDbEntry["year"].get<std::string>();
                }
            } else {
                vpsYear = "";
            }
            vpsManufacturer = vpsDbEntry.value("manufacturer", "");

            if (vpsName.empty()) {
                LOG_DEBUG("VpsDatabaseClient: Skipping VPSDB entry with empty 'name'. ID: " << vpsId);
                continue;
            }

            // Get the latest version from this VPSDB entry's files
            if (vpsDbEntry.contains("tableFiles") && vpsDbEntry["tableFiles"].is_array()) {
                for (const auto& file : vpsDbEntry["tableFiles"]) {
                    if (file.value("tableFormat", "") == "VPX") {
                        std::string vpsFileVersion = file.value("version", "");
                        if (isVersionGreaterThan(vpsFileVersion, currentVpsEntryLatestVersion)) {
                             currentVpsEntryLatestVersion = vpsFileVersion;
                        }
                    }
                }
            }
        } catch (const nlohmann::json::exception& e) {
            LOG_DEBUG("VpsDatabaseClient: JSON parsing error within vpsdb entry (ID: " << vpsDbEntry.value("id", "N/A") << "): " << e.what());
            continue;
        } catch (const std::exception& e) {
            LOG_DEBUG("VpsDatabaseClient: Error parsing basic vpsdb entry info (ID: " << vpsDbEntry.value("id", "N/A") << "): " << e.what());
            continue;
        }
        
        std::string normVpsNameAggressive = normalizeString(vpsName);
        std::string normVpsNameLessAggressive = normalizeStringLessAggressive(vpsName);
        
        int currentMatchScore = -1;

        // Matching priority:
        // 1. Aggressive Table Name Match (most precise)
        if (!normVpxTableNameAggressive.empty() && normVpxTableNameAggressive == normVpsNameAggressive) {
            currentMatchScore = 3;
            LOG_DEBUG("VpsDatabaseClient: Potential match (Score 3 - Aggressive Name): VPX '" << tableData.tableName << "' <-> VPSDB '" << vpsName << "' (VPS ID: " << vpsId << ")");
        }
        // 2. Less Aggressive Table Name Match
        else if (!normVpxTableNameLessAggressive.empty() && normVpxTableNameLessAggressive == normVpsNameLessAggressive) {
            currentMatchScore = 2;
            LOG_DEBUG("VpsDatabaseClient: Potential match (Score 2 - Less Aggressive Name): VPX '" << tableData.tableName << "' <-> VPSDB '" << vpsName << "' (VPS ID: " << vpsId << ")");
        }
        // 3. ROM Name Match (crucial for mods/conversions like Yellow Submarine, DuckTales)
        else if (!normVpxGameName.empty() && (normVpxGameName == normVpsNameAggressive || normVpxGameName == normVpsNameLessAggressive)) {
            currentMatchScore = 1;
            LOG_DEBUG("VpsDatabaseClient: Potential match (Score 1 - Game Name): VPX ROM '" << tableData.romPath << "' <-> VPSDB '" << vpsName << "' (VPS ID: " << vpsId << ")");
        }
        // 4. Year and Manufacturer Match (only if NO name/ROM matches were found, and only if *both* are present and match)
        else if (bestMatchScore < 1 && // Only consider this if no ROM or name match has been found yet
                   !tableData.year.empty() && !tableData.manufacturer.empty() &&
                   tableData.year == vpsYear && normalizeStringLessAggressive(tableData.manufacturer) == normalizeStringLessAggressive(vpsManufacturer)) {
            currentMatchScore = 0;
            LOG_DEBUG("VpsDatabaseClient: Potential match (Score 0 - Year/Manufacturer): VPX '" << tableData.tableName << "' (Y:" << tableData.year << ", M:" << tableData.manufacturer << ") <-> VPSDB '" << vpsName << "' (Y:" << vpsYear << ", M:" << vpsManufacturer << ") (VPS ID: " << vpsId << ")");
        }

        // If this is a better match, or an equally good match but with a newer version, select it.
        if (currentMatchScore > bestMatchScore) {
            bestMatchScore = currentMatchScore;
            bestVpsDbEntry = vpsDbEntry;
            latestVpsVersionFound = currentVpsEntryLatestVersion;
            bestMatchVpsName = vpsName; // Store the VPS name of the best match
            matched_to_vpsdb_entry = true;
        } else if (currentMatchScore == bestMatchScore && currentMatchScore > -1) {
            // If scores are equal, prefer the one with the latest version
            if (isVersionGreaterThan(currentVpsEntryLatestVersion, latestVpsVersionFound)) {
                latestVpsVersionFound = currentVpsEntryLatestVersion;
                bestVpsDbEntry = vpsDbEntry; // Update to this entry if its version is newer
                bestMatchVpsName = vpsName;
                matched_to_vpsdb_entry = true;
            }
        }
    }

    // --- Phase 3: Apply enrichment from the BEST match found ---
    if (matched_to_vpsdb_entry) {
        // Always populate VPS-specific fields
        tableData.vpsId = bestVpsDbEntry.value("id", "");
        tableData.vpsName = bestVpsDbEntry.value("name", "");
        tableData.type = bestVpsDbEntry.value("type", "");
        tableData.themes = bestVpsDbEntry.contains("theme") ? join(bestVpsDbEntry["theme"], ", ") : "";
        tableData.designers = bestVpsDbEntry.contains("designers") ? join(bestVpsDbEntry["designers"], ", ") : "";
        tableData.players = bestVpsDbEntry.contains("players") && bestVpsDbEntry["players"].is_number_integer() ? std::to_string(bestVpsDbEntry["players"].get<int>()) : "";
        tableData.ipdbUrl = bestVpsDbEntry.value("ipdbUrl", "");
        
        // Update manufacturer/year ONLY if VPSDB has more specific info or current is generic/empty,
        // AND for score 0 matches, only update if the current data is truly generic.
        if (!bestVpsDbEntry.value("manufacturer", "").empty() && 
            (tableData.manufacturer.empty() || normalizeStringLessAggressive(tableData.manufacturer) == "original" || bestMatchScore > 0)) {
            tableData.manufacturer = bestVpsDbEntry.value("manufacturer", "");
        }
        if (!bestVpsDbEntry.value("year", "").empty() && (tableData.year.empty() || bestMatchScore > 0)) {
            if (bestVpsDbEntry["year"].is_number_integer()) {
                tableData.year = std::to_string(bestVpsDbEntry["year"].get<int>());
            } else if (bestVpsDbEntry["year"].is_string()) {
                tableData.year = bestVpsDbEntry["year"].get<std::string>();
            }
        }

        // Iterate through tableFiles of the BEST match to get authors, features, and the most relevant comment
        if (bestVpsDbEntry.contains("tableFiles") && bestVpsDbEntry["tableFiles"].is_array()) {
            for (const auto& file : bestVpsDbEntry["tableFiles"]) {
                if (file.value("tableFormat", "") == "VPX") {
                    tableData.vpsAuthors = file.contains("authors") ? join(file["authors"], ", ") : "";
                    tableData.features = file.contains("features") ? join(file["features"], ", ") : "";
                    // Prioritize VPS comment if current description is empty or shorter
                    std::string current_vps_comment = file.value("comment", "");
                    if (!current_vps_comment.empty()) {
                        if (tableData.tableDescription.empty() || current_vps_comment.length() > tableData.tableDescription.length()) {
                            tableData.tableDescription = current_vps_comment;
                        }
                    }
                    tableData.vpsComment = current_vps_comment; // Store the last relevant comment
                }
            }
        }
        
        // Set the primary display title (tableData.title) from vpsName if a strong match was found.
        // This is crucial for mods/conversions where ROM name dictates the actual table name.
        if (bestMatchScore >= 1 && !bestMatchVpsName.empty()) {
            tableData.title = bestMatchVpsName;
        }
        // If it was a weak (score 0) match, *do not* overwrite the title.
        // It retains the filename-derived title from TableLoader.
        // If bestMatchScore was -1 (no match), title remains filename-derived.


        // Update table version display
        std::string currentVpxVersionNormalized = normalizeVersion(vpxTableVersion_source);
        tableData.vpsVersion = latestVpsVersionFound; // Store the latest VPS version found

        if (!latestVpsVersionFound.empty() && isVersionGreaterThan(latestVpsVersionFound, currentVpxVersionNormalized)) {
            if (!currentVpxVersionNormalized.empty()) {
                tableData.tableVersion = currentVpxVersionNormalized + " (Latest: " + latestVpsVersionFound + ")";
            } else {
                tableData.tableVersion = "(Latest: " + latestVpsVersionFound + ")";
            }
            LOG_INFO("VpsDatabaseClient: Updated table '" << tableData.title << "' with latest VPSDB version info: " << tableData.tableVersion);
        } else if (!currentVpxVersionNormalized.empty()) {
            tableData.tableVersion = currentVpxVersionNormalized;
        } else if (!latestVpsVersionFound.empty()) {
            tableData.tableVersion = latestVpsVersionFound;
        }
        // If all versions are empty, tableData.tableVersion remains empty.


        LOG_INFO("VpsDatabaseClient: Successfully enriched table '" << tableData.title << "' (Original VPX Table Name: '" << vpxTableName_source << "') overall with VPSDB info. Final VPX Version Display: '" << tableData.tableVersion << "'");
    } else {
        // If no strong match, title remains the filename-derived title.
        // Log the details for debugging missing matches.
        LOG_DEBUG("VpsDatabaseClient: No strong vpsdb match found for table: '" << tableData.tableName << "', gameName: '" << tableData.romPath << "', VPX Current Version: '" << tableData.tableVersion << "', Year: '" << tableData.year << "', Manufacturer: '" << tableData.manufacturer << "'.");
        mismatchLog << "No strong vpsdb match for table: " << tableData.tableName << ", gameName: " << tableData.romPath << ", VPX Current Version: " << tableData.tableVersion << ", Year: " << tableData.year << ", Manufacturer: " << tableData.manufacturer << "\n";
    }

    return matched_to_vpsdb_entry;
}

bool VpsDatabaseClient::fetchIfNeeded(const std::string& lastUpdatedPath, const std::string& updateFrequency) {
    if (updateFrequency != "startup") {
        LOG_INFO("VpsDatabaseClient: VpsDb update skipped, frequency set to: " << updateFrequency);
        return fs::exists(vpsDbPath_);
    }

    std::vector<std::string> vpsDbUrls = {
        "https://virtualpinballspreadsheet.github.io/vps-db/db/vpsdb.json"
        // we can add more url fallbacks here
    };
    std::string lastUpdatedUrl = "https://virtualpinballspreadsheet.github.io/vps-db/lastUpdated.json";

    try {
        std::ifstream lastUpdatedFile(lastUpdatedPath);
        long localTimestamp = 0;
        if (lastUpdatedFile.is_open()) {
            nlohmann::json lastUpdatedJson;
            lastUpdatedFile >> lastUpdatedJson;
            if (lastUpdatedJson.contains("updatedAt")) {
                if (lastUpdatedJson["updatedAt"].is_number()) {
                    localTimestamp = lastUpdatedJson["updatedAt"].get<long>();
                } else if (lastUpdatedJson["updatedAt"].is_string()) {
                    try {
                        localTimestamp = std::stol(lastUpdatedJson["updatedAt"].get<std::string>());
                    } catch (const std::exception& e) {
                        LOG_DEBUG("VpsDatabaseClient: Invalid updatedAt string format: " << e.what());
                    }
                }
            }
            lastUpdatedFile.close();
        }

        std::string lastUpdatedContent, lastUpdatedHeaders;
        long httpCode = 0;
        CURL* curl = curl_easy_init();
        if (curl) {
            curl_easy_setopt(curl, CURLOPT_URL, lastUpdatedUrl.c_str());
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &lastUpdatedContent);
            curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, headerCallback);
            curl_easy_setopt(curl, CURLOPT_HEADERDATA, &lastUpdatedHeaders);
            CURLcode res = curl_easy_perform(curl);
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
            curl_easy_cleanup(curl);
            if (res != CURLE_OK) {
                LOG_ERROR("VpsDatabaseClient: Failed to fetch lastUpdated.json: " << curl_easy_strerror(res));
                return fs::exists(vpsDbPath_);
            }
            if (httpCode != 200) {
                LOG_ERROR("VpsDatabaseClient: Failed to fetch lastUpdated.json, HTTP status: " << httpCode);
                return fs::exists(vpsDbPath_);
            }
            // Check content-type (allow optional parameters like charset)
            if (lastUpdatedHeaders.find("application/json") == std::string::npos) {
                LOG_ERROR("VpsDatabaseClient: lastUpdated.json has invalid content-type, headers: " << lastUpdatedHeaders);
                return fs::exists(vpsDbPath_);
            }
            // Log first 100 chars of content for debugging
            LOG_DEBUG("VpsDatabaseClient: lastUpdated.json content (first 100 chars): " << lastUpdatedContent.substr(0, 100));
        } else {
            LOG_ERROR("VpsDatabaseClient: Failed to initialize curl");
            return fs::exists(vpsDbPath_);
        }

        nlohmann::json remoteLastUpdated;
        try {
            remoteLastUpdated = nlohmann::json::parse(lastUpdatedContent);
        } catch (const std::exception& e) {
            LOG_ERROR("VpsDatabaseClient: Failed to parse remote lastUpdated.json: " << e.what());
            return fs::exists(vpsDbPath_);
        }
        
        long remoteTimestamp = 0;
        if (remoteLastUpdated.is_number()) {
            remoteTimestamp = remoteLastUpdated.get<long>();
        } else if (remoteLastUpdated.is_object() && remoteLastUpdated.contains("updatedAt")) {
            if (remoteLastUpdated["updatedAt"].is_number()) {
                remoteTimestamp = remoteLastUpdated["updatedAt"].get<long>();
            } else if (remoteLastUpdated["updatedAt"].is_string()) {
                try {
                    remoteTimestamp = std::stol(remoteLastUpdated["updatedAt"].get<std::string>());
                } catch (const std::exception& e) {
                    LOG_DEBUG("VpsDatabaseClient: Invalid remote updatedAt string format: " << e.what());
                }
            }
        } else {
            LOG_ERROR("VpsDatabaseClient: Invalid lastUpdated.json format; expected number or object with 'updatedAt'");
            return fs::exists(vpsDbPath_);
        }

        if (remoteTimestamp > localTimestamp || !fs::exists(vpsDbPath_)) {
            bool downloadSuccess = false;
            std::string vpsDbContent, vpsDbHeaders;
            for (const auto& url : vpsDbUrls) {
                httpCode = 0;
                vpsDbContent.clear();
                vpsDbHeaders.clear();
                curl = curl_easy_init();
                if (curl) {
                    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
                    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
                    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &vpsDbContent);
                    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, headerCallback);
                    curl_easy_setopt(curl, CURLOPT_HEADERDATA, &vpsDbHeaders);
                    CURLcode res = curl_easy_perform(curl);
                    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
                    curl_easy_cleanup(curl);
                    if (res != CURLE_OK) {
                        LOG_ERROR("VpsDatabaseClient: Failed to download vpsdb.json from " << url << ": " << curl_easy_strerror(res));
                        continue;
                    }
                    if (httpCode != 200) {
                        LOG_ERROR("VpsDatabaseClient: Failed to download vpsdb.json from " << url << ", HTTP status: " << httpCode);
                        continue;
                    }
                    if (vpsDbHeaders.find("application/json") == std::string::npos) {
                        LOG_ERROR("VpsDatabaseClient: vpsdb.json from " << url << " has invalid content-type, headers: " << vpsDbHeaders);
                        continue;
                    }
                    // Log first 100 chars of content
                    LOG_DEBUG("VpsDatabaseClient: vpsdb.json content (first 100 chars) from " << url << ": " << vpsDbContent.substr(0, 100));
                    // Validate JSON
                    try {
                        nlohmann::json parsed = nlohmann::json::parse(vpsDbContent);
                    } catch (const std::exception& e) {
                        LOG_ERROR("VpsDatabaseClient: Downloaded vpsdb.json from " << url << " is invalid JSON: " << e.what());
                        continue;
                    }
                    // Save vpsdb.json
                    try {
                        fs::create_directories(fs::path(vpsDbPath_).parent_path());
                        std::ofstream out(vpsDbPath_);
                        if (!out.is_open()) {
                            LOG_ERROR("VpsDatabaseClient: Failed to open " << vpsDbPath_ << " for writing");
                            continue;
                        }
                        out << vpsDbContent;
                        out.close();
                        downloadSuccess = true;
                        break;
                    } catch (const std::exception& e) {
                        LOG_ERROR("VpsDatabaseClient: Failed to save vpsdb.json: " << e.what());
                        continue;
                    }
                } else {
                    LOG_ERROR("VpsDatabaseClient: Failed to initialize curl for " << url);
                    continue;
                }
            }

            if (downloadSuccess) {
                // Save lastUpdated.json
                try {
                    std::ofstream lastUpdatedOut(lastUpdatedPath);
                    if (!lastUpdatedOut.is_open()) {
                        LOG_ERROR("VpsDatabaseClient: Failed to open " << lastUpdatedPath << " for writing");
                        return true; // vpsdb.json was saved, so proceed
                    }
                    lastUpdatedOut << remoteLastUpdated.dump();
                    lastUpdatedOut.close();
                    LOG_INFO("VpsDatabaseClient: Updated vpsdb.json and lastUpdated.json");
                } catch (const std::exception& e) {
                    LOG_ERROR("VpsDatabaseClient: Failed to save lastUpdated.json: " << e.what());
                    return true; // vpsdb.json was saved, so proceed
                }
            } else {
                LOG_ERROR("VpsDatabaseClient: Failed to download valid vpsdb.json from all URLs");
                return fs::exists(vpsDbPath_);
            }
        } else {
            LOG_INFO("VpsDatabaseClient: vpsdb.json is up-to-date");
        }
    } catch (const std::exception& e) {
        LOG_ERROR("VpsDatabaseClient: Error checking vpsdb update: " << e.what());
        return fs::exists(vpsDbPath_);
    }
    return true;
}