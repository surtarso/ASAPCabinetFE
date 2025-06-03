#include "vps_data_enricher.h"
#include <fstream>
#include <regex>
#include "utils/logging.h"

VpsDataEnricher::VpsDataEnricher(const nlohmann::json& vpsDb) : vpsDb_(vpsDb) {}

bool VpsDataEnricher::enrichTableData(const nlohmann::json& vpxTable, TableData& tableData) const {
    if (!vpxTable.is_object()) {
        LOG_DEBUG("VpsDataEnricher: vpxTable is not an object, type: " << vpxTable.type_name());
        return false;
    }

    std::string vpxTableName_source, vpxGameName_source, vpxTableVersion_source, vpxReleaseDate_source, vpxAuthorName_source, vpxTableDescription_source;

    try {
        if (vpxTable.contains("table_info") && vpxTable["table_info"].is_object()) {
            const auto& tableInfo = vpxTable["table_info"];
            if (tableInfo.contains("table_name") && tableInfo["table_name"].is_string()) {
                vpxTableName_source = tableInfo["table_name"].get<std::string>();
            }
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
        if (vpxTable.contains("game_name") && vpxTable["game_name"].is_string()) {
            vpxGameName_source = vpxTable["game_name"].get<std::string>();
        }

        if (!vpxTableName_source.empty()) {
            tableData.tableName = vpxTableName_source;
        }
        if (tableData.tableName.empty()) {
            tableData.tableName = tableData.title;
        }
        tableData.tableVersion = vpxTableVersion_source;
        tableData.authorName = vpxAuthorName_source;
        tableData.tableDescription = vpxTableDescription_source;
        tableData.romPath = vpxGameName_source;
        tableData.gameName = vpxGameName_source;

        if (!vpxReleaseDate_source.empty()) {
            tableData.year = utils_.extractYearFromDate(vpxReleaseDate_source);
        }
        if (tableData.year.empty() && !tableData.tableName.empty()) {
            std::regex yearFromTitleRegex("\\((\\d{4})\\)");
            std::smatch match;
            if (std::regex_search(tableData.tableName, match, yearFromTitleRegex)) {
                tableData.year = match[1].str();
            }
        }
        if (tableData.manufacturer.empty() && !tableData.tableName.empty()) {
            std::regex manufFromTitleRegex("\\(([^)]+?)(?:\\s+\\d{4})?\\)");
            std::smatch match;
            if (std::regex_search(tableData.tableName, match, manufFromTitleRegex)) {
                tableData.manufacturer = match[1].str();
            }
        }
    } catch (const std::exception& e) {
        LOG_DEBUG("VpsDataEnricher: Error processing table data: " << vpxTable.value("path", "N/A") << ": " << e.what());
    }

    std::ofstream mismatchLog("tables/vpsdb_mismatches.log", std::ios::app);
    bool matched_to_vpsdb = false;
    std::string latestVpsVersionFound = "";
    nlohmann::json bestVpsDbEntry = {};
    std::string bestMatchVpsName = "";
    
    std::string normVpxTableNameAggressive = utils_.normalizeString(tableData.tableName);
    std::string normVpxTableNameLessAggressive = utils_.normalizeStringLessAggressive(tableData.tableName);
    std::string normVpxGameName = utils_.normalizeString(tableData.romPath);
    
    LOG_DEBUG("VpsDataEnricher: Attempting to enrich table '" << tableData.tableName
        << "' (Normalized: '" << normVpxTableNameAggressive << "', Less Aggressive: '" << normVpxTableNameLessAggressive
        << "') with Game Name '" << tableData.romPath << "' (Normalized: '" << normVpxGameName << "')"
        << ", Year: '" << tableData.year << "', Manufacturer: '" << tableData.manufacturer
        << "', Version: '" << tableData.tableVersion << "'");

    int bestMatchScore = -1;

    for (const auto& vpsDbEntry : vpsDb_) {
        std::string vpsId = vpsDbEntry.value("id", "N/A");
        std::string vpsName = vpsDbEntry.value("name", "");
        std::string vpsYear;
        std::string vpsManufacturer = vpsDbEntry.value("manufacturer", "");
        std::string currentVpsEntryLatestVersion = "";

        try {
            if (vpsDbEntry.contains("year")) {
                if (vpsDbEntry["year"].is_number_integer()) {
                    vpsYear = std::to_string(vpsDbEntry["year"].get<int>());
                } else if (vpsDbEntry["year"].is_string()) {
                    vpsYear = vpsDbEntry["year"].get<std::string>();
                }
            }
            if (vpsName.empty()) {
                LOG_DEBUG("VpsDataEnricher: Skipping VPSDB entry with empty 'name'. ID: " << vpsId);
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
        } catch (const std::exception& e) {
            LOG_DEBUG("VpsDataEnricher: Error parsing vpsdb entry (ID: " << vpsDbEntry.value("id", "N/A") << "): " << e.what());
            continue;
        }

        std::string normVpsNameAggressive = utils_.normalizeString(vpsName);
        std::string normVpsNameLessAggressive = utils_.normalizeStringLessAggressive(vpsName);

        int currentMatchScore = -1;

        if (!normVpxTableNameAggressive.empty() && normVpxTableNameAggressive == normVpsNameAggressive) {
            currentMatchScore = 3;
            LOG_DEBUG("VpsDataEnricher: Potential match (Score 3 - Aggressive Name): VPX '" << tableData.tableName << "' <-> VPSDB '" << vpsName << "' (VPS ID: " << vpsId << ")");
        } else if (!normVpxTableNameLessAggressive.empty() && normVpxTableNameLessAggressive == normVpsNameLessAggressive) {
            currentMatchScore = 2;
            LOG_DEBUG("VpsDataEnricher: Potential match (Score 2 - Less Aggressive Name): VPX '" << tableData.tableName << "' <-> VPSDB '" << vpsName << "' (VPS ID: " << vpsId << ")");
        } else if (!normVpxGameName.empty() && (normVpxGameName == normVpsNameAggressive || normVpxGameName == normVpsNameLessAggressive)) {
            currentMatchScore = 1;
            LOG_DEBUG("VpsDataEnricher: Potential match (Score 1 - Game Name): VPX ROM '" << tableData.romPath << "' <-> VPSDB '" << vpsName << "' (VPS ID: " << vpsId << ")");
        } else if (bestMatchScore < 1 && !tableData.year.empty() && !tableData.manufacturer.empty() &&
                   tableData.year == vpsYear && utils_.normalizeStringLessAggressive(tableData.manufacturer) == utils_.normalizeStringLessAggressive(vpsManufacturer)) {
            currentMatchScore = 0;
            LOG_DEBUG("VpsDataEnricher: Potential match (Score 0 - Year/Manufacturer): VPX '" << tableData.tableName << "' (Y:" << tableData.year << ", M:" << tableData.manufacturer << ") <-> VPSDB '" << vpsName << "' (Y:" << vpsYear << ", M:" << vpsManufacturer << ") (VPS ID: " << vpsId << ")");
        }

        if (currentMatchScore > bestMatchScore) {
            bestMatchScore = currentMatchScore;
            bestVpsDbEntry = vpsDbEntry;
            latestVpsVersionFound = currentVpsEntryLatestVersion;
            bestMatchVpsName = vpsName;
            matched_to_vpsdb = true;
        } else if (currentMatchScore == bestMatchScore && currentMatchScore > -1) {
            if (utils_.isVersionGreaterThan(currentVpsEntryLatestVersion, latestVpsVersionFound)) {
                latestVpsVersionFound = currentVpsEntryLatestVersion;
                bestVpsDbEntry = vpsDbEntry;
                bestMatchVpsName = vpsName;
                matched_to_vpsdb = true;
            }
        }
    }

    if (matched_to_vpsdb) {
        tableData.vpsId = bestVpsDbEntry.value("id", "");
        tableData.vpsName = bestVpsDbEntry.value("name", "");
        tableData.type = bestVpsDbEntry.value("type", "");
        tableData.themes = bestVpsDbEntry.contains("theme") ? utils_.join(bestVpsDbEntry["theme"], ", ") : "";
        tableData.designers = bestVpsDbEntry.contains("designers") ? utils_.join(bestVpsDbEntry["designers"], ", ") : "";
        tableData.players = bestVpsDbEntry.contains("players") && bestVpsDbEntry["players"].is_number_integer() ? std::to_string(bestVpsDbEntry["players"].get<int>()) : "";
        tableData.ipdbUrl = bestVpsDbEntry.value("ipdbUrl", "");

        if (!bestVpsDbEntry.value("manufacturer", "").empty() && 
            (tableData.manufacturer.empty() || utils_.normalizeStringLessAggressive(tableData.manufacturer) == "original" || bestMatchScore > 0)) {
            tableData.manufacturer = bestVpsDbEntry.value("manufacturer", "");
        }
        if (!bestVpsDbEntry.value("year", "").empty() && (tableData.year.empty() || bestMatchScore > 0)) {
            if (bestVpsDbEntry["year"].is_number_integer()) {
                tableData.year = std::to_string(bestVpsDbEntry["year"].get<int>());
            } else if (bestVpsDbEntry["year"].is_string()) {
                tableData.year = bestVpsDbEntry["year"].get<std::string>();
            }
        }

        if (bestVpsDbEntry.contains("tableFiles") && bestVpsDbEntry["tableFiles"].is_array()) {
            for (const auto& file : bestVpsDbEntry["tableFiles"]) {
                if (file.value("tableFormat", "") == "VPX") {
                    tableData.vpsAuthors = file.contains("authors") ? utils_.join(file["authors"], ", ") : "";
                    tableData.features = file.contains("features") ? utils_.join(file["features"], ", ") : "";
                    std::string current_vps_comment = file.value("comment", "");
                    if (!current_vps_comment.empty()) {
                        if (tableData.tableDescription.empty() || current_vps_comment.length() > tableData.tableDescription.length()) {
                            tableData.tableDescription = current_vps_comment;
                        }
                    }
                    tableData.vpsComment = current_vps_comment;
                }
            }
        }

        if (bestMatchScore >= 1 && !bestMatchVpsName.empty()) {
            tableData.title = bestMatchVpsName;
        }

        std::string currentVpxVersionNormalized = utils_.normalizeVersion(vpxTableVersion_source);
        tableData.vpsVersion = latestVpsVersionFound;

        if (!latestVpsVersionFound.empty() && utils_.isVersionGreaterThan(latestVpsVersionFound, currentVpxVersionNormalized)) {
            if (!currentVpxVersionNormalized.empty()) {
                tableData.tableVersion = currentVpxVersionNormalized + " (Latest: " + latestVpsVersionFound + ")";
            } else {
                tableData.tableVersion = "(Latest: " + latestVpsVersionFound + ")";
            }
            LOG_INFO("VpsDataEnricher: Updated table '" << tableData.title << "' with latest VPSDB version info: " << tableData.tableVersion);
        } else if (!currentVpxVersionNormalized.empty()) {
            tableData.tableVersion = currentVpxVersionNormalized;
        } else if (!latestVpsVersionFound.empty()) {
            tableData.tableVersion = latestVpsVersionFound;
        }

        LOG_INFO("VpsDataEnricher: Successfully enriched table '" << tableData.title << "' with VPSDB info");
    } else {
        LOG_DEBUG("VpsDataEnricher: No vpsdb match for table: '" << tableData.tableName << "', gameName: '" << tableData.romPath << "'");
        mismatchLog << "No vpsdb match for table: " << tableData.tableName << ", gameName: " << tableData.romPath << "\n";
    }

    return matched_to_vpsdb;
}