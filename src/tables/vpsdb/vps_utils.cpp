/**
 * @file vps_utils.cpp
 * @brief Implements the VpsUtils class for utility functions in ASAPCabinetFE.
 *
 * This file provides the implementation of the VpsUtils class, which offers utility
 * methods for string normalization, version comparison, date parsing, and JSON array
 * joining. These methods support VPS metadata processing in ASAPCabinetFE, ensuring
 * consistency in matching and enrichment tasks. The functionality can be extended
 * with configUI for user-defined normalization or formatting rules in the future.
 */

#include "vps_utils.h"
#include <regex>
#include <algorithm>
#include <cctype>
#include "utils/logging.h"

std::string VpsUtils::join(const nlohmann::json& array, const std::string& delimiter) const {
    std::vector<std::string> items;
    for (const auto& item : array) {
        try {
            if (item.is_string()) {
                items.push_back(item.get<std::string>());
            }
        } catch (const std::exception& e) {
            LOG_DEBUG("VpsUtils: Skipping invalid array item in join: " << e.what());
        }
    }
    // Use accumulate to join strings with delimiter, starting with the first item
    return items.empty() ? "" : std::accumulate(std::next(items.begin()), items.end(), items[0],
        [&delimiter](const std::string& a, const std::string& b) { return a + delimiter + b; });
}

std::string VpsUtils::normalizeString(const std::string& input) const {
    std::string result = input;
    std::transform(result.begin(), result.end(), result.begin(), ::tolower); // Convert to lowercase
    result.erase(std::remove_if(result.begin(), result.end(), [](char c) {
        return !std::isalnum(c); // Remove non-alphanumeric characters
    }), result.end());
    return result;
}

std::string VpsUtils::normalizeStringLessAggressive(const std::string& input) const {
    std::string result = input;
    std::transform(result.begin(), result.end(), result.begin(), ::tolower); // Convert to lowercase
    // Remove specific punctuation, preserve spaces and parentheses
    result.erase(std::remove_if(result.begin(), result.end(), [](char c) {
        return (c == '_' || c == '-' || c == '.' || c == '\'' || c == ',' || c == '!' || c == '?' || c == ':' || c == '&');
    }), result.end());

    // Collapse multiple spaces to single space
    std::string cleaned_result;
    std::unique_copy(result.begin(), result.end(), std::back_inserter(cleaned_result),
                     [](char a, char b) { return std::isspace(a) && std::isspace(b); });

    // Trim leading/trailing spaces
    size_t first = cleaned_result.find_first_not_of(' ');
    if (std::string::npos == first) return "";
    size_t last = cleaned_result.find_last_not_of(' ');
    return cleaned_result.substr(first, (last - first + 1));
}

std::string VpsUtils::normalizeVersion(const std::string& version) const {
    std::string normalized = version;
    std::replace(normalized.begin(), normalized.end(), ',', '.'); // Replace commas with dots
    normalized.erase(0, normalized.find_first_not_of(" \t\n\r\f\v")); // Trim leading whitespace
    normalized.erase(normalized.find_last_not_of(" \t\n\r\f\v") + 1); // Trim trailing whitespace

    size_t dash_pos = normalized.find('-');
    if (dash_pos != std::string::npos) {
        std::string first_part = normalized.substr(0, dash_pos);
        if (std::regex_match(first_part, std::regex("[0-9\\.]+"))) { // Check if first part is numeric (e.g., "1.2-beta")
            return first_part;
        }
    }
    return normalized;
}

bool VpsUtils::isVersionGreaterThan(const std::string& v1, const std::string& v2) const {
    std::string norm_v1 = normalizeVersion(v1);
    std::string norm_v2 = normalizeVersion(v2);

    if (norm_v1.empty()) return false; // Empty v1 is not greater
    if (norm_v2.empty()) return true; // Non-empty v1 is greater than empty v2

    try {
        float f1 = std::stof(norm_v1); // Convert to float for numeric comparison
        float f2 = std::stof(norm_v2);
        return f1 > f2;
    } catch (const std::exception&) {
        return norm_v1 > norm_v2; // Fallback to lexicographical comparison if conversion fails
    }
}

std::string VpsUtils::extractYearFromDate(const std::string& dateString) const {
    if (dateString.empty()) {
        return "";
    }

    // Step 1: Trim whitespace
    std::string normalized = dateString;
    normalized.erase(0, normalized.find_first_not_of(" \t\n\r\f\v"));
    normalized.erase(normalized.find_last_not_of(" \t\n\r\f\v") + 1);
    if (normalized.empty()) {
        return "";
    }

    // Step 2: Normalize separators (replace commas, slashes, hyphens with dots)
    std::replace_if(normalized.begin(), normalized.end(),
        [](char c) { return c == ',' || c == '/' || c == '-'; }, '.');

    // Step 3: Match common date formats with 4-digit year
    std::regex dateRegex_DDMMYYYY(R"(\b(\d{1,2})\.(\d{1,2})\.(\d{4})\b)"); // DD.MM.YYYY or D.M.YYYY
    std::regex dateRegex_YYYYMMDD(R"(\b(\d{4})\.\d{1,2}\.\d{1,2}\b)"); // YYYY.MM.DD
    std::smatch match;

    if (std::regex_search(normalized, match, dateRegex_DDMMYYYY) && match.size() == 4) {
        return match[3].str(); // Extract year from DD.MM.YYYY
    }
    if (std::regex_search(normalized, match, dateRegex_YYYYMMDD) && match.size() == 2) {
        return match[1].str(); // Extract year from YYYY.MM.DD
    }

    // Step 4: Match standalone 4-digit year (1900–2100)
    std::regex yearRegex(R"(\b(19\d{2}|20\d{2})\b)");
    if (std::regex_search(normalized, match, yearRegex)) {
        return match[1].str();
    }

    // Step 5: Match short date formats with 2-digit year (e.g., DD.MM.YY)
    std::regex dateRegex_DDMMYY(R"(\b\d{1,2})\d{1,2}\.(\d{2})\b)");
    if (std::regex_search(normalized, match, dateRegex_DDMMYY) && match.size() == 4) {
        std::string yy = match[3].str();
        int year = std::stoi(yy);
        // Assume 00-69 -> 2000-2069, 70-99 -> 1970-1999
        return (year >= 70 ? "19" : "20") + yy;
    }

    // Step 6: Fallback - search for any 4-digit number in 1900-2100-2100 range
    for (size_t i = 0; i + 4 <= normalized.length(); i++) {
        if (std::isdigit(normalized[i])) {
            std::string potential_year = normalized.substr(i, 4);
            if (potential_year.length() == 4 && std::all_of(potential_year.begin(), potential_year.end(), ::isdigit)) {
                int year = std::stoi(potential_year);
                if (year >= 1900 && year <= 2100) {
                    return potential_year;
                }
            }
        }
    }

    // Step 7: No valid year found
    return "";
}
