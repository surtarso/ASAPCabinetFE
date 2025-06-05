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
    std::regex dateRegex_DDMMYYYY("\\d{2}\\.\\d{2}\\.(\\d{4})"); // Match DD.MM.YYYY format
    std::regex dateRegex_YYYY("\\d{4}"); // Match standalone YYYY format
    std::smatch match;

    if (std::regex_search(dateString, match, dateRegex_DDMMYYYY) && match.size() > 1) {
        return match[1].str(); // Extract year from DD.MM.YYYY
    } else if (std::regex_search(dateString, match, dateRegex_YYYY) && match.size() > 0) {
        return match[0].str(); // Extract standalone year
    }
    return "";
}