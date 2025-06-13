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

bool VpsDataScanner::matchMetadata(const nlohmann::json& vpxTable, TableData& tableData, LoadingProgress* progress) const {
    LOG_INFO("Started matchmaking for table path: " << vpxTable.value("path", "N/A") << ", title=" << tableData.title << ", tableName=" << tableData.tableName << ", manufacturer=" << tableData.manufacturer << ", year=" << tableData.year);

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

    // --- Match Score Weights (Adjust these values to tweak matching behavior) ---
    const float ROM_MATCH_WEIGHT = 0.2f;
    const float ROM_MATCH_REMAKE_WEIGHT = 0.05f;
    const float TITLE_EXACT_AGGRESSIVE_WEIGHT = 5.0f;
    const float TITLE_EXACT_LESS_AGGRESSIVE_WEIGHT = 4.0f;
    const float TITLE_LEVENSHTEIN_BASE_WEIGHT = 1.5f;
    const float TABLENAME_EXACT_AGGRESSIVE_WEIGHT = 2.0f;
    const float TABLENAME_EXACT_LESS_AGGRESSIVE_WEIGHT = 1.0f;
    const float TABLENAME_LEVENSHTEIN_BASE_WEIGHT = 1.0f;
    const float FILENAME_EXACT_WEIGHT = 2.0f;
    const float FILENAME_LEVENSHTEIN_BASE_WEIGHT = 1.0f;
    const float YEAR_MATCH_WEIGHT = 1.0f;
    const float MANUFACTURER_MATCH_WEIGHT = 1.0f;
    const float AUTHOR_MATCH_WEIGHT = 0.7f;
    const float TYPE_MATCH_WEIGHT = 0.5f;
    // Weights for Description and Rules
    const float DESCRIPTION_WORD_OVERLAP_WEIGHT = 0.5f; // General word overlap
    const float RULES_WORD_OVERLAP_WEIGHT = 0.7f;      // Rules are often more specific

    // Matching Thresholds
    const int MIN_MATCHING_FIELDS = 2; // Allow 1 if score is high enough
    const float MIN_MATCH_SCORE_STRICT = 5.0f; // For very confident matches (e.g., exact title + year/manuf)
    const float MIN_MATCH_SCORE_LENIENT = 2.0f; // For less confident matches that are still acceptable
    const float LEVENSHTEIN_THRESHOLD = 0.8f; // Similarity for accepting Levenshtein matches
    const float LEVENSHTEIN_REJECT_THRESHOLD = 0.7f; // Lower threshold for logging rejection

    // --- Populate TableData fields from VPin internal metadata (these are *separate* from filename-derived) ---
    // These fields should *always* be populated from vpxTable if available, regardless of prior values.
    if (vpxTable.contains("table_info") && vpxTable["table_info"].is_object()) {
        const auto& tableInfo = vpxTable["table_info"];
        tableData.tableName = utils_.cleanString(utils_.safeGetString(tableInfo, "table_name", ""));
        tableData.tableAuthor = utils_.cleanString(utils_.safeGetString(tableInfo, "author_name", ""));
        tableData.tableDescription = utils_.cleanString(utils_.safeGetString(tableInfo, "table_description", ""));
        tableData.tableSaveDate = utils_.safeGetString(tableInfo, "table_save_date", "");
        tableData.tableReleaseDate = utils_.safeGetString(tableInfo, "release_date", "");
        tableData.tableVersion = utils_.cleanString(utils_.safeGetString(tableInfo, "table_version", ""));
        tableData.tableRevision = utils_.safeGetString(tableInfo, "table_save_rev", "");
        tableData.tableBlurb = utils_.cleanString(utils_.safeGetString(tableInfo, "table_blurb", ""));
        tableData.tableRules = utils_.cleanString(utils_.safeGetString(tableInfo, "table_rules", ""));
        tableData.tableAuthorEmail = utils_.cleanString(utils_.safeGetString(tableInfo, "author_email", ""));
        tableData.tableAuthorWebsite = utils_.cleanString(utils_.safeGetString(tableInfo, "author_website", ""));

        if (vpxTable.contains("properties") && vpxTable["properties"].is_object()) {
            const auto& properties = vpxTable["properties"];
            tableData.tableType = utils_.cleanString(utils_.safeGetString(properties, "TableType", ""));
            tableData.tableManufacturer = utils_.cleanString(utils_.safeGetString(properties, "CompanyName", 
                                                         utils_.safeGetString(properties, "Company", "")));
            tableData.tableYear = utils_.cleanString(utils_.safeGetString(properties, "CompanyYear", 
                                                         utils_.safeGetString(properties, "Year", "")));
        }
    }
    // ROM name from VPin scan. Only set if FileScanner didn't find one (FileScanner is more reliable).
    if (tableData.romName.empty()) {
        tableData.romName = vpxTable.value("rom", "");
    }
    // Note: tableData.romPath is set by FileScanner and should not be overwritten here.
    LOG_DEBUG("VPin metadata populated. table.tableName=" << tableData.tableName << ", table.tableManufacturer=" << tableData.tableManufacturer << ", table.tableYear=" << tableData.tableYear << ", table.romName=" << tableData.romName);

    // Populate generic 'title', 'year', 'manufacturer' for comparison ---
    // 1. Get cleaned versions of potential titles using the new utility function
    std::string cleanedFilenameTitle = utils_.extractCleanTitle(filename);
    std::string cleanedTableNameTitle = utils_.extractCleanTitle(tableData.tableName);
    std::string tempTitleForMatching;

    // Prioritize how tableData.title is set for matching purposes:
    // If tableName is substantial and gives a 'clean' result, prefer it.
    // "Substantial" means at least 3 characters long after cleaning.
    // Compare lengths to avoid picking a greatly truncated tableName over a potentially better filename.
    if (!tableData.tableName.empty() && cleanedTableNameTitle.length() >= 3 &&
        (cleanedTableNameTitle.length() >= cleanedFilenameTitle.length() - 2 || cleanedFilenameTitle.empty())) { // Prefer tableName unless filename is significantly better
        tempTitleForMatching = cleanedTableNameTitle;
        LOG_DEBUG("Chosen tempTitleForMatching from cleaned tableName: " << tempTitleForMatching);
    } else {
        tempTitleForMatching = cleanedFilenameTitle;
        LOG_DEBUG("Chosen tempTitleForMatching from cleaned filename: " << tempTitleForMatching);
    }
    
    // THIS PRODUCES LOTS OF FALSE POSITIVES!
    // Final check: if the chosen title is very short and we have a romName, maybe use romName as a last resort?
    // This helps for cases like filename "2024" or tableName "Test" with a real ROM.
    // if (tempTitleForMatching.length() < 3 && !tableData.romName.empty()) {
    //     tempTitleForMatching = tableData.romName;
    //     LOG_DEBUG("Chosen tempTitleForMatching from romName due to very short title: " << tempTitleForMatching);
    // }

    // Set the final title used for matching against VPSDB entries.
    tableData.title = tempTitleForMatching;
    LOG_DEBUG("Final tableData.title for matching set to: " << tableData.title);

    // Manufacturer and Year extraction: Prioritize internal VPX metadata, then specific filename patterns, then generic parsing.
    // IMPORTANT: These still operate on the original filename and metadata, not the `cleanedFilenameTitle`,
    // because `extractCleanTitle` removes manufacturer/year patterns that are needed here.
    
    // Manufacturer
    if (tableData.manufacturer.empty()) {
        if (!tableData.tableManufacturer.empty()) {
            tableData.manufacturer = tableData.tableManufacturer;
            LOG_DEBUG("Set generic manufacturer from VPin metadata: " << tableData.manufacturer);
        } else {
            // Original logic for (Manufacturer Year) from filename
            std::regex manufYearRegex(R"(\(([^)]+?)(?:\s+(\d{4}))?\))"); // Capture Manufacturer and optional Year
            std::smatch match;
            if (std::regex_search(filename, match, manufYearRegex)) {
                tableData.manufacturer = match[1].str();
                LOG_DEBUG("Set generic manufacturer from filename pattern: " << tableData.manufacturer);
                if (match.size() > 2 && !match[2].str().empty()) {
                    tableData.year = match[2].str();
                    LOG_DEBUG("Set generic year from filename manufacturer pattern: " << tableData.year);
                }
            }
        }
    }

    // Year (if not already set by manufacturer pattern or VPin metadata)
    if (tableData.year.empty()) {
        if (!tableData.tableYear.empty()) {
            tableData.year = tableData.tableYear;
            LOG_DEBUG("Set generic year from VPin metadata: " << tableData.year);
        } else {
            tableData.year = utils_.extractYearFromDate(filename);
            if (!tableData.year.empty()) {
                LOG_DEBUG("Set generic year from filename (general 4-digit): " << tableData.year);
            }
        }
    }
    // End of initial population / fallback for generic fields


    bool matched_to_vpsdb = false;
    nlohmann::json bestVpsDbEntry;
    std::string latestVpsVersionFound;
    std::string bestMatchVpsName;
    float bestMatchScore = -1.0f;
    int bestMatchingFields = 0;

    std::string normTitleAggressive = utils_.normalizeString(tableData.title);
    std::string normTitleLessAggressive = utils_.normalizeStringLessAggressive(tableData.title);
    std::string normTableNameAggressive = utils_.normalizeString(tableData.tableName);
    std::string normTableNameLessAggressive = utils_.normalizeStringLessAggressive(tableData.tableName);
    std::string normGameName = utils_.normalizeString(tableData.romName);
    std::string normFilenameAggressive = utils_.normalizeString(filename); // Aggressive for filename too
    std::string normFilenameLessAggressive = utils_.normalizeStringLessAggressive(filename);
     // Normalized rules and description for word overlap matching
    std::string normTableRules = utils_.normalizeStringLessAggressive(tableData.tableRules);
    std::string normTableDescription = utils_.normalizeStringLessAggressive(tableData.tableDescription);


    LOG_DEBUG("Attempting to match table: title=" << tableData.title << ", tableName=" << tableData.tableName << ", romName=" << tableData.romName << ", filename=" << filename);

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
            int currentMatchingFields = 0;
            bool isRemake = false;
            bool romMatched = false; // Declared here, per loop iteration

            // Check for remake in vpsComment and vpsName itself
            if (vpsDbEntry.contains("tableFiles") && vpsDbEntry["tableFiles"].is_array()) {
                for (const auto& file : vpsDbEntry["tableFiles"]) {
                    std::string comment = file.value("comment", "");
                    if (!comment.empty() && (comment.find("Retheme") != std::string::npos || comment.find("mod") != std::string::npos)) {
                        isRemake = true;
                        LOG_DEBUG("Detected remake/mod in vpsComment: " << comment);
                        break; // Found it, no need to check other files
                    }
                }
            }
            // Also check if the VPS name itself contains common remake/mod indicators
            std::string lowerVpsName = utils_.toLower(vpsName);
            if (lowerVpsName.find("mod") != std::string::npos || 
                lowerVpsName.find("retheme") != std::string::npos ||
                lowerVpsName.find("recreation") != std::string::npos) {
                isRemake = true;
                LOG_DEBUG("Detected remake/mod in vpsName: " << vpsName);
            }

            // ROM match (low weight, lower for remakes)
            if (!normGameName.empty() && vpsDbEntry.contains("tableFiles") && vpsDbEntry["tableFiles"].is_array()) {
                for (const auto& tableFile : vpsDbEntry["tableFiles"]) {
                    if (tableFile.contains("roms") && tableFile["roms"].is_array()) {
                        for (const auto& rom : tableFile["roms"]) {
                            if (rom.contains("name") && rom["name"].is_string()) {
                                std::string romName = rom["name"].get<std::string>();
                                if (!romName.empty() && utils_.normalizeString(romName) == normGameName) {
                                    currentMatchScore += isRemake ? ROM_MATCH_REMAKE_WEIGHT : ROM_MATCH_WEIGHT;
                                    currentMatchingFields++;
                                    romMatched = true; // Set romMatched flag
                                    LOG_DEBUG("ROM match found for: " << tableData.romName << ", score added=" << (isRemake ? ROM_MATCH_REMAKE_WEIGHT : ROM_MATCH_WEIGHT));
                                    goto next_rom_check; // Found ROM match, break from inner loops
                                }
                            }
                        }
                    }
                }
                next_rom_check:; // Label for goto
            }

            // Title match (highest weight)
            float nameSimilarityTitle = 0.0f;
            if (!normTitleAggressive.empty() && normTitleAggressive == normVpsNameAggressive) {
                nameSimilarityTitle = TITLE_EXACT_AGGRESSIVE_WEIGHT;
                currentMatchingFields++;
                LOG_DEBUG("Exact title match (aggressive): " << tableData.title << ", score=" << TITLE_EXACT_AGGRESSIVE_WEIGHT);
            } else if (!normTitleLessAggressive.empty() && normTitleLessAggressive == normVpsNameLessAggressive) {
                nameSimilarityTitle = TITLE_EXACT_LESS_AGGRESSIVE_WEIGHT;
                currentMatchingFields++;
                LOG_DEBUG("Exact title match (less aggressive): " << tableData.title << ", score=" << TITLE_EXACT_LESS_AGGRESSIVE_WEIGHT);
            } else if (!normTitleLessAggressive.empty()) {
                // Try Levenshtein with both aggressive and less aggressive VPS names
                size_t distAggressive = utils_.levenshteinDistance(normTitleLessAggressive, normVpsNameAggressive);
                size_t distLessAggressive = utils_.levenshteinDistance(normTitleLessAggressive, normVpsNameLessAggressive);

                float simAggressive = 1.0f - static_cast<float>(distAggressive) / std::max(normTitleLessAggressive.size(), normVpsNameAggressive.size());
                float simLessAggressive = 1.0f - static_cast<float>(distLessAggressive) / std::max(normTitleLessAggressive.size(), normVpsNameLessAggressive.size());

                float bestSim = std::max(simAggressive, simLessAggressive);

                if (bestSim >= LEVENSHTEIN_THRESHOLD) {
                    nameSimilarityTitle = bestSim * TITLE_LEVENSHTEIN_BASE_WEIGHT;
                    currentMatchingFields++;
                    LOG_DEBUG("Levenshtein title match, similarity=" << bestSim << ", score=" << nameSimilarityTitle);
                } else if (bestSim >= LEVENSHTEIN_REJECT_THRESHOLD) {
                    LOG_DEBUG("Near title match rejected, similarity=" << bestSim << ", vpsName=" << vpsName);
                }
            }

            // TableName match
            float nameSimilarityTableName = 0.0f;
            if (!normTableNameAggressive.empty() && normTableNameAggressive == normVpsNameAggressive) {
                nameSimilarityTableName = TABLENAME_EXACT_AGGRESSIVE_WEIGHT;
                currentMatchingFields++;
                LOG_DEBUG("Exact tableName match (aggressive): " << tableData.tableName << ", score=" << TABLENAME_EXACT_AGGRESSIVE_WEIGHT);
            } else if (!normTableNameLessAggressive.empty() && normTableNameLessAggressive == normVpsNameLessAggressive) {
                nameSimilarityTableName = TABLENAME_EXACT_LESS_AGGRESSIVE_WEIGHT;
                currentMatchingFields++;
                LOG_DEBUG("Exact tableName match (less aggressive): " << tableData.tableName << ", score=" << TABLENAME_EXACT_LESS_AGGRESSIVE_WEIGHT);
            } else if (!normTableNameLessAggressive.empty()) {
                // Try Levenshtein with both aggressive and less aggressive VPS names
                size_t distAggressive = utils_.levenshteinDistance(normTableNameLessAggressive, normVpsNameAggressive);
                size_t distLessAggressive = utils_.levenshteinDistance(normTableNameLessAggressive, normVpsNameLessAggressive);

                float simAggressive = 1.0f - static_cast<float>(distAggressive) / std::max(normTableNameLessAggressive.size(), normVpsNameAggressive.size());
                float simLessAggressive = 1.0f - static_cast<float>(distLessAggressive) / std::max(normTableNameLessAggressive.size(), normVpsNameLessAggressive.size());

                float bestSim = std::max(simAggressive, simLessAggressive);

                if (bestSim >= LEVENSHTEIN_THRESHOLD) {
                    nameSimilarityTableName = bestSim * TABLENAME_LEVENSHTEIN_BASE_WEIGHT;
                    currentMatchingFields++;
                    LOG_DEBUG("Levenshtein tableName match, similarity=" << bestSim << ", score=" << nameSimilarityTableName);
                } else if (bestSim >= LEVENSHTEIN_REJECT_THRESHOLD) {
                    LOG_DEBUG("Near tableName match rejected, similarity=" << bestSim << ", vpsName=" << vpsName);
                }
            }

            // Filename match
            float nameSimilarityFilename = 0.0f;
            if (!normFilenameLessAggressive.empty()) {
                if (normFilenameAggressive == normVpsNameAggressive || normFilenameLessAggressive == normVpsNameLessAggressive) {
                    nameSimilarityFilename = FILENAME_EXACT_WEIGHT;
                    currentMatchingFields++;
                    LOG_DEBUG("Filename match (exact/fuzzy): " << filename << ", score=" << nameSimilarityFilename);
                } else {
                    size_t distAggressive = utils_.levenshteinDistance(normFilenameLessAggressive, normVpsNameAggressive);
                    size_t distLessAggressive = utils_.levenshteinDistance(normFilenameLessAggressive, normVpsNameLessAggressive);

                    float simAggressive = 1.0f - static_cast<float>(distAggressive) / std::max(normFilenameLessAggressive.size(), normVpsNameAggressive.size());
                    float simLessAggressive = 1.0f - static_cast<float>(distLessAggressive) / std::max(normFilenameLessAggressive.size(), normVpsNameLessAggressive.size());

                    float bestSim = std::max(simAggressive, simLessAggressive);

                    if (bestSim >= LEVENSHTEIN_THRESHOLD) {
                        nameSimilarityFilename = bestSim * FILENAME_LEVENSHTEIN_BASE_WEIGHT;
                        currentMatchingFields++;
                        LOG_DEBUG("Levenshtein filename match, similarity=" << bestSim << ", score=" << nameSimilarityFilename);
                    } else if (bestSim >= LEVENSHTEIN_REJECT_THRESHOLD) {
                        LOG_DEBUG("Near filename match rejected, similarity=" << bestSim << ", vpsName=" << vpsName);
                    }
                }
            }

            // Combine name scores: Add the score from the *best* name match (title, tableName, or filename)
            // This ensures only the strongest name similarity contributes its full weight, preventing accumulation of weak name matches.
            float bestCombinedNameScore = std::max({nameSimilarityTitle, nameSimilarityTableName, nameSimilarityFilename});
            currentMatchScore += bestCombinedNameScore; // This line might be redundant if the score is already factored in other name similarities.
                                                        // Let's refine this to ensure the *highest* name score is added only once.
                                                        // The initial `currentMatchScore += nameSimilarityTitle;` etc. were removed.
                                                        // So, adding `bestCombinedNameScore` here is correct.

            if (nameSimilarityTitle == bestCombinedNameScore) {
                LOG_DEBUG("Title match contributed most to name score: title=" << tableData.title);
            } else if (nameSimilarityTableName == bestCombinedNameScore) {
                LOG_DEBUG("TableName match contributed most to name score: tableName=" << tableData.tableName);
            } else if (nameSimilarityFilename == bestCombinedNameScore) {
                LOG_DEBUG("Filename match contributed most to name score: filename=" << filename);
            }

            // Year match
            bool yearMatched = false;
            if (!tableData.year.empty() && !vpsYear.empty() && tableData.year == vpsYear) {
                currentMatchScore += YEAR_MATCH_WEIGHT;
                currentMatchingFields++;
                yearMatched = true;
                LOG_DEBUG("Year match (filename/derived): " << tableData.year << ", score added=" << YEAR_MATCH_WEIGHT);
            } else if (!tableData.tableYear.empty() && !vpsYear.empty() && tableData.tableYear == vpsYear) {
                // Only add if no primary year match, or if primary year was empty
                currentMatchScore += (YEAR_MATCH_WEIGHT * 0.5f); // Half weight for VPin internal year
                currentMatchingFields++;
                yearMatched = true;
                LOG_DEBUG("TableYear match (VPin internal): " << tableData.tableYear << ", score added=" << (YEAR_MATCH_WEIGHT * 0.5f));
            }

            // Manufacturer match
            bool manufacturerMatched = false;
            if (!tableData.manufacturer.empty() && !vpsManufacturer.empty() &&
                utils_.normalizeStringLessAggressive(tableData.manufacturer) == utils_.normalizeStringLessAggressive(vpsManufacturer)) {
                currentMatchScore += MANUFACTURER_MATCH_WEIGHT;
                currentMatchingFields++;
                manufacturerMatched = true;
                LOG_DEBUG("Manufacturer match (filename/derived): " << tableData.manufacturer << ", score added=" << MANUFACTURER_MATCH_WEIGHT);
            } else if (!tableData.tableManufacturer.empty() && !vpsManufacturer.empty() &&
                       utils_.normalizeStringLessAggressive(tableData.tableManufacturer) == utils_.normalizeStringLessAggressive(vpsManufacturer)) {
                // Only add if no primary manufacturer match, or if primary manufacturer was empty
                currentMatchScore += (MANUFACTURER_MATCH_WEIGHT * 0.5f); // Half weight for VPin internal manufacturer
                currentMatchingFields++;
                manufacturerMatched = true;
                LOG_DEBUG("TableManufacturer match (VPin internal): " << tableData.tableManufacturer << ", score added=" << (MANUFACTURER_MATCH_WEIGHT * 0.5f));
            }

            // Author/Designer match (tableAuthor vs vpsDesigners/vpsAuthors)
            // Combine all authors/designers from VPSDB into a set for easier lookup
            std::set<std::string> vpsAllAuthors;
            if (vpsDbEntry.contains("designers") && vpsDbEntry["designers"].is_array()) {
                for (const auto& designer : vpsDbEntry["designers"]) {
                    if (designer.is_string()) {
                        vpsAllAuthors.insert(utils_.normalizeStringLessAggressive(designer.get<std::string>()));
                    }
                }
            }
            if (vpsDbEntry.contains("tableFiles") && vpsDbEntry["tableFiles"].is_array()) {
                for (const auto& file : vpsDbEntry["tableFiles"]) {
                    if (file.contains("authors") && file["authors"].is_array()) {
                        for (const auto& author : file["authors"]) {
                            if (author.is_string()) {
                                vpsAllAuthors.insert(utils_.normalizeStringLessAggressive(author.get<std::string>()));
                            }
                        }
                    }
                }
            }
            bool authorMatched = false;
            if (!tableData.tableAuthor.empty() && !vpsAllAuthors.empty()) {
                std::string normTableAuthor = utils_.normalizeStringLessAggressive(tableData.tableAuthor);
                // Check if table author is one of the VPS authors/designers or has significant word overlap
                if (vpsAllAuthors.count(normTableAuthor) > 0 || hasWordOverlap(normTableAuthor, utils_.join(vpsDbEntry.contains("designers") ? vpsDbEntry["designers"] : nlohmann::json::array(), " "), utils_) || hasWordOverlap(normTableAuthor, utils_.join(vpsDbEntry.contains("tableFiles") && !vpsDbEntry["tableFiles"].empty() ? vpsDbEntry["tableFiles"][0].value("authors", nlohmann::json::array()) : nlohmann::json::array(), " "), utils_)) {
                    currentMatchScore += AUTHOR_MATCH_WEIGHT;
                    currentMatchingFields++;
                    authorMatched = true;
                    LOG_DEBUG("Author/Designer match: " << tableData.tableAuthor << ", score added=" << AUTHOR_MATCH_WEIGHT);
                }
            }
            
            // Table Type match (tableType vs vpsType)
            bool typeMatched = false;
            if (!tableData.tableType.empty() && vpsDbEntry.contains("type") && vpsDbEntry["type"].is_string()) {
                // Direct normalized string match for common types like "SS", "EM", etc.
                if (utils_.normalizeString(tableData.tableType) == utils_.normalizeString(vpsDbEntry["type"].get<std::string>())) {
                    currentMatchScore += TYPE_MATCH_WEIGHT;
                    currentMatchingFields++;
                    typeMatched = true;
                    LOG_DEBUG("Type match: " << tableData.tableType << ", score added=" << TYPE_MATCH_WEIGHT);
                }
            }
            
            // Rules and Description Word Overlap Matches
            if (!normTableRules.empty() && vpsDbEntry.contains("description") && vpsDbEntry["description"].is_string()) {
                std::string normVpsDescription = utils_.normalizeStringLessAggressive(vpsDbEntry["description"].get<std::string>());
                if (hasWordOverlap(normTableRules, normVpsDescription, utils_)) {
                    currentMatchScore += RULES_WORD_OVERLAP_WEIGHT;
                    currentMatchingFields++;
                    LOG_DEBUG("Rules-Description word overlap match, score added=" << RULES_WORD_OVERLAP_WEIGHT);
                }
            }
            if (!normTableDescription.empty() && vpsDbEntry.contains("description") && vpsDbEntry["description"].is_string()) {
                std::string normVpsDescription = utils_.normalizeStringLessAggressive(vpsDbEntry["description"].get<std::string>());
                if (hasWordOverlap(normTableDescription, normVpsDescription, utils_)) {
                    currentMatchScore += DESCRIPTION_WORD_OVERLAP_WEIGHT;
                    currentMatchingFields++;
                    LOG_DEBUG("Description-Description word overlap match, score added=" << DESCRIPTION_WORD_OVERLAP_WEIGHT);
                }
            }
            // Also check for word overlap of vpsName with table rules/description
            if (!normVpsNameLessAggressive.empty()) {
                if (!normTableRules.empty() && hasWordOverlap(normVpsNameLessAggressive, normTableRules, utils_)) {
                    currentMatchScore += (RULES_WORD_OVERLAP_WEIGHT * 0.75f); // Slightly less weight
                    currentMatchingFields++;
                    LOG_DEBUG("VPSName-Rules word overlap match, score added=" << (RULES_WORD_OVERLAP_WEIGHT * 0.75f));
                }
                if (!normTableDescription.empty() && hasWordOverlap(normVpsNameLessAggressive, normTableDescription, utils_)) {
                    currentMatchScore += (DESCRIPTION_WORD_OVERLAP_WEIGHT * 0.75f); // Slightly less weight
                    currentMatchingFields++;
                    LOG_DEBUG("VPSName-Description word overlap match, score added=" << (DESCRIPTION_WORD_OVERLAP_WEIGHT * 0.75f));
                }
            }

            // --- Final Match Decision Logic ---
            // This is the core area for false positive reduction.
            // A good match should have:
            // 1. A strong name match (title, tableName, or filename)
            // 2. OR a very strong combination of other distinct features.

            bool isConfidentMatch = false;

            // Scenario 1: Very strong name match (exact title/tableName/filename)
            if (bestCombinedNameScore >= TITLE_EXACT_AGGRESSIVE_WEIGHT || // An exact title match (aggressive)
                bestCombinedNameScore >= TITLE_EXACT_LESS_AGGRESSIVE_WEIGHT) { // Or an exact title (less aggressive)
                isConfidentMatch = true;
                LOG_DEBUG("Confident match via strong name similarity.");
            }
            // Scenario 2: Strong name match (Levenshtein based) PLUS another distinct strong feature (Year, Manufacturer, or Author)
            else if (bestCombinedNameScore >= (LEVENSHTEIN_THRESHOLD * TITLE_LEVENSHTEIN_BASE_WEIGHT) && // Levenshtein title/tableName/filename match
                     (yearMatched || manufacturerMatched || authorMatched)) { // AND at least one of these
                isConfidentMatch = true;
                LOG_DEBUG("Confident match via strong name Levenshtein + secondary feature.");
            }
            // Scenario 3: ROM match PLUS a strong secondary feature (Year, Manufacturer, or Author)
            // This needs to be carefully considered due to ROM ambiguity. Perhaps only allow if remake=false.
            else if (romMatched && !isRemake && // Has a ROM and is not a remake
                     currentMatchScore >= ROM_MATCH_WEIGHT && // And the ROM contributed to the score
                     (yearMatched || manufacturerMatched || authorMatched)) { // And has another strong secondary match
                isConfidentMatch = true;
                LOG_DEBUG("Confident match via ROM + secondary feature (non-remake).");
            }
            // Scenario 4: Multiple very specific non-name matches (e.g., Year + Manufacturer + Author + Type)
            else if (yearMatched && manufacturerMatched && authorMatched && typeMatched) {
                isConfidentMatch = true;
                LOG_DEBUG("Confident match via multiple specific non-name features.");
            }
            // NEW Scenario 5: Significant word overlap in rules/description + a primary name match (even fuzzy)
            else if ((normTableRules.length() > 10 || normTableDescription.length() > 10) && // Ensure text is substantial
                     (bestCombinedNameScore > 0 || romMatched) && // Needs some name or rom basis
                     (currentMatchScore >= (RULES_WORD_OVERLAP_WEIGHT + DESCRIPTION_WORD_OVERLAP_WEIGHT))) { // Sufficient overlap score
                isConfidentMatch = true;
                LOG_DEBUG("Confident match via rules/description overlap + name/ROM basis.");
            }
            
            // Apply a minimum score, even if confident by other rules, to avoid trivial matches
            if (isConfidentMatch && currentMatchScore < MIN_MATCH_SCORE_LENIENT) {
                isConfidentMatch = false; // Override if total score is too low
                LOG_DEBUG("Confident match downgraded due to low total score: " << currentMatchScore);
            }

            // Consolidated Best Match Decision Logic:
            // An entry qualifies as a candidate for the best match if:
            // 1. It is a 'confident match' based on the detailed scenarios above, OR
            // 2. It has a very strict minimum total score, OR
            // 3. It meets the minimum number of matching fields AND has *some* positive contribution from a name (title/tableName/filename) or ROM match.
            bool isCandidateForBestMatch = isConfidentMatch ||
                                           (currentMatchScore >= MIN_MATCH_SCORE_STRICT) ||
                                           ((currentMatchingFields >= MIN_MATCHING_FIELDS) && (bestCombinedNameScore > 0 || romMatched));

            if (isCandidateForBestMatch && currentMatchScore > bestMatchScore) {
                bestMatchScore = currentMatchScore;
                bestVpsDbEntry = vpsDbEntry;
                latestVpsVersionFound = currentVpsEntryLatestVersion;
                bestMatchVpsName = vpsName;
                matched_to_vpsdb = true; // Set to true if any match is found
                bestMatchingFields = currentMatchingFields;
                LOG_INFO("New best match candidate found. Score=" << bestMatchScore << ", fields=" << currentMatchingFields << ", vpsName=" << vpsName << ", isRemake=" << isRemake << ", Confident=" << isConfidentMatch);
            } else if (!isCandidateForBestMatch && currentMatchScore >= MIN_MATCH_SCORE_LENIENT) {
                // Log potential near matches that didn't qualify as a "candidate for best match"
                LOG_DEBUG("Near match rejected (score=" << currentMatchScore << ", fields=" << currentMatchingFields << ", vpsName=" << vpsName << ", currentBestScore=" << bestMatchScore << ")");
            } else if (!isCandidateForBestMatch && currentMatchScore < MIN_MATCH_SCORE_LENIENT && currentMatchScore > 0) {
                // Log very weak matches, mostly for debugging
                LOG_DEBUG("Very weak match rejected (score=" << currentMatchScore << ", fields=" << currentMatchingFields << ", vpsName=" << vpsName << ")");
            }

        } catch (const std::exception& e) {
            LOG_DEBUG("Error in VPSDB entry ID=" << vpsDbEntry.value("id", "N/A") << ": " << e.what());
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
        tableData.vpsName = bestMatchVpsName; // The VPSDB's canonical name
        tableData.vpsType = bestVpsDbEntry.value("type", "");
        tableData.vpsThemes = bestVpsDbEntry.contains("theme") && bestVpsDbEntry["theme"].is_array() ? utils_.join(bestVpsDbEntry["theme"], ", ") : "";
        tableData.vpsDesigners = bestVpsDbEntry.contains("designers") && bestVpsDbEntry["designers"].is_array() ? utils_.join(bestVpsDbEntry["designers"], ", ") : "";
        tableData.vpsPlayers = bestVpsDbEntry.contains("players") && !bestVpsDbEntry["players"].is_null() && bestVpsDbEntry["players"].is_number_integer() ? std::to_string(bestVpsDbEntry["players"].get<int>()) : "";
        tableData.vpsIpdbUrl = bestVpsDbEntry.value("ipdbUrl", "");
        tableData.vpsManufacturer = bestVpsDbEntry.value("manufacturer", "");
        tableData.vpsYear = bestVpsDbEntry.contains("year") && !bestVpsDbEntry["year"].is_null() ?
                            (bestVpsDbEntry["year"].is_number_integer() ? std::to_string(bestVpsDbEntry["year"].get<int>()) :
                             bestVpsDbEntry["year"].is_string() ? bestVpsDbEntry["year"].get<std::string>() : "") : "";

        // Populate general manufacturer and year from VPSDB *only if they are currently empty*.
        // This preserves filename/VPin metadata if they were already present.
        if (tableData.manufacturer.empty() && !tableData.vpsManufacturer.empty()) {
            tableData.manufacturer = tableData.vpsManufacturer;
            LOG_DEBUG("Updated generic manufacturer from VPSDB: " << tableData.vpsManufacturer);
        }
        if (tableData.year.empty() && !tableData.vpsYear.empty()) {
            tableData.year = tableData.vpsYear;
            LOG_DEBUG("Updated generic year from VPSDB: " << tableData.vpsYear);
        }

        // Get playfield links and other table-specific info from VPSDB
        tableData.vpsTableImgUrl = ""; // Reset before finding
        tableData.vpsTableUrl = ""; // Reset before finding
        tableData.vpsAuthors = ""; // Reset before finding
        tableData.vpsFeatures = ""; // Reset before finding
        tableData.vpsComment = ""; // Reset before finding

        if (bestVpsDbEntry.contains("tableFiles") && bestVpsDbEntry["tableFiles"].is_array()) {
            for (const auto& file : bestVpsDbEntry["tableFiles"]) {
                tableData.vpsFormat = file.value("tableFormat", "");
                if (file.contains("imgUrl") && file["imgUrl"].is_string()) {
                    tableData.vpsTableImgUrl = file["imgUrl"].get<std::string>();
                    LOG_DEBUG("Set vpsTableImgUrl: " << tableData.vpsTableImgUrl);
                }
                if (file.contains("urls") && file["urls"].is_array() && !file["urls"].empty()) {
                    if (file["urls"][0].contains("url") && file["urls"][0]["url"].is_string()) {
                        tableData.vpsTableUrl = file["urls"][0]["url"].get<std::string>();
                        LOG_DEBUG("Set vpsTableUrl: " << tableData.vpsTableUrl);
                    }
                }
                // Append authors and features as they can be multiple per tableFile entry
                if (file.contains("authors") && file["authors"].is_array()) {
                    std::string newAuthors = utils_.join(file["authors"], ", ");
                    if (!tableData.vpsAuthors.empty() && !newAuthors.empty()) tableData.vpsAuthors += ", ";
                    tableData.vpsAuthors += newAuthors;
                }
                if (file.contains("features") && file["features"].is_array()) {
                    std::string newFeatures = utils_.join(file["features"], ", ");
                    if (!tableData.vpsFeatures.empty() && !newFeatures.empty()) tableData.vpsFeatures += ", ";
                    tableData.vpsFeatures += newFeatures;
                }
                if (file.contains("comment") && file["comment"].is_string()) {
                    std::string newComment = file["comment"].get<std::string>();
                    if (!tableData.vpsComment.empty() && !newComment.empty()) tableData.vpsComment += "; ";
                    tableData.vpsComment += newComment;
                }
            }
        }

        // Get backglass links from VPSDB
        tableData.vpsB2SImgUrl = ""; // Reset
        tableData.vpsB2SUrl = ""; // Reset
        if (bestVpsDbEntry.contains("b2sFiles") && bestVpsDbEntry["b2sFiles"].is_array()) {
            for (const auto& file : bestVpsDbEntry["b2sFiles"]) {
                if (file.contains("imgUrl") && file["imgUrl"].is_string()) {
                    tableData.vpsB2SImgUrl = file["imgUrl"].get<std::string>();
                    LOG_DEBUG("Set vpsB2SImgUrl: " << tableData.vpsB2SImgUrl);
                }
                if (file.contains("urls") && file["urls"].is_array() && !file["urls"].empty()) {
                    if (file["urls"][0].contains("url") && file["urls"][0]["url"].is_string()) {
                        tableData.vpsB2SUrl = file["urls"][0]["url"].get<std::string>();
                        LOG_DEBUG("Set vpsB2SUrl: " << tableData.vpsB2SUrl);
                    }
                }
            }
        }

        // Handle version information: tableData.vpsVersion stores the latest from VPSDB.
        // tableData.tableVersion is for the original VPX file version, possibly augmented.
        std::string currentVpxVersionNormalized = utils_.normalizeVersion(tableData.tableVersion);
        tableData.vpsVersion = latestVpsVersionFound; // This correctly holds the VPSDB version
        
        // You can keep the logic for updating tableData.tableVersion to reflect latest from VPSDB.
        // This is for *metadata comparison*, not the primary display title.
        if (!latestVpsVersionFound.empty() && utils_.isVersionGreaterThan(latestVpsVersionFound, currentVpxVersionNormalized)) {
            if (!currentVpxVersionNormalized.empty()) {
                tableData.tableVersion = currentVpxVersionNormalized + " (Latest: " + latestVpsVersionFound + ")";
            } else {
                tableData.tableVersion = "(Latest: " + latestVpsVersionFound + ")";
            }
            LOG_DEBUG("Updated tableVersion to include VPSDB latest: " << tableData.tableVersion);
        } else if (!currentVpxVersionNormalized.empty()) {
            tableData.tableVersion = currentVpxVersionNormalized;
        } else if (!latestVpsVersionFound.empty()) {
            tableData.tableVersion = latestVpsVersionFound;
        }

        tableData.matchConfidence = (bestMatchScore / 10);
        tableData.jsonOwner = "Virtual Pinball Spreadsheet";
        LOG_INFO("Matched table " << tableData.vpsName << " to VPSDB, confidence: " << tableData.matchConfidence << ", fields: " << bestMatchingFields);

        if (progress) {
            std::lock_guard<std::mutex> lock(progress->mutex);
            progress->numMatched++;
        }
    } else {
        // If no VPSDB match, ensure tableData.title is from tableName or filename for display fallback.
        // This logic remains as you preferred not to override outside of explicit VPSDB match.
        tableData.title = tableData.tableName.empty() ? filename : tableData.tableName;
        {
            std::lock_guard<std::mutex> lock(mismatchLogMutex);
            std::ofstream mismatchLog("logs/vpsdb_mismatches.log", std::ios::app);
            mismatchLog << "No VPSDB match for table: title='" << tableData.title << "', tableName='" << tableData.tableName
                        << "', romName='" << tableData.romName << "', filename='" << filename
                        << "', year='" << tableData.year << "', manufacturer='" << tableData.manufacturer
                        << "', tableYear='" << tableData.tableYear << "', tableManufacturer='" << tableData.tableManufacturer << "'\n";
        }
        LOG_INFO("No VPSDB match, using title: " << tableData.title);
        if (progress) {
            std::lock_guard<std::mutex> lock(progress->mutex);
            progress->numNoMatch++;
        }

        // Final fallback for generic year and manufacturer if still empty
        if (!filename.empty()) {
            if (tableData.year.empty()) {
                tableData.year = utils_.extractYearFromDate(filename);
                if (!tableData.year.empty()) {
                    LOG_DEBUG("Final fallback year (from filename): " << tableData.year);
                } else {
                    std::regex yearRegex(R"(\b(\d{4})\b)");
                    std::smatch match;
                    if (std::regex_search(filename, match, yearRegex)) {
                        tableData.year = match[1].str();
                        LOG_DEBUG("Final fallback year (regex from filename): " << tableData.year);
                    }
                }
            }
            if (tableData.manufacturer.empty()) {
                std::regex manufRegex(R"(\(([^)]+?)(?:\s+\d{4})?\))");
                std::smatch match;
                if (std::regex_search(filename, match, manufRegex)) {
                    tableData.manufacturer = match[1].str();
                    LOG_DEBUG("Final fallback manufacturer (from filename): " << tableData.manufacturer);
                }
            }
        }
    }

    LOG_DEBUG("Final table state after VPSDB match attempt: title=" << tableData.title << ", tableName=" << tableData.tableName << ", vpsName=" << tableData.vpsName
              << ", manufacturer=" << tableData.manufacturer << ", vpsManufacturer=" << tableData.vpsManufacturer
              << ", year=" << tableData.year << ", vpsYear=" << tableData.vpsYear);
    return matched_to_vpsdb;
}