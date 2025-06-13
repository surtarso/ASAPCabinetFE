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

#include "vps_utils.h"      // Include your own header for VpsUtils
#include "utils/logging.h"  // Assuming this is your logging header
#include <regex>            // For std::regex
#include <algorithm>        // For std::transform, std::remove_if, std::replace_if, std::all_of
#include <cctype>           // For ::tolower, ::isalnum, ::isdigit, ::isspace
#include <numeric>          // For std::accumulate (for std::next)
#include <vector>           // For std::vector
#include <string>           // For std::string
#include <stdexcept>        // For std::exception, std::stof
#include <locale>
#include <sstream>

// VpsUtils::normalizeString
std::string VpsUtils::normalizeString(const std::string& input) const {
    std::string result = input;
    // Convert to lowercase
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c){ return static_cast<unsigned char>(std::tolower(c)); });
    // Remove non-alphanumeric characters
    result.erase(std::remove_if(result.begin(), result.end(),
                                [](unsigned char c) { return !std::isalnum(c); }),
                 result.end());
    return result;
}

// VpsUtils::normalizeStringLessAggressive
std::string VpsUtils::normalizeStringLessAggressive(const std::string& input) const {
    std::string result = input;
    // Convert to lowercase
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c){ return static_cast<unsigned char>(std::tolower(c)); });

    // Remove specific punctuation, preserve spaces and parentheses
    result.erase(std::remove_if(result.begin(), result.end(), [](char c) {
        return (c == '_' || c == '-' || c == '.' || c == '\'' || c == ',' || c == '!' || c == '?' || c == ':' || c == '&');
    }), result.end());

    // Collapse multiple spaces to single space and trim leading/trailing spaces
    std::string cleaned_result;
    bool last_char_was_space = true; // Treat start as if previous was a space for leading trim
    for (char c : result) {
        if (std::isspace(static_cast<unsigned char>(c))) {
            if (!last_char_was_space) {
                cleaned_result += ' ';
                last_char_was_space = true;
            }
        } else {
            cleaned_result += c;
            last_char_was_space = false;
        }
    }
    // Trim trailing space if the last character added was a space
    if (!cleaned_result.empty() && cleaned_result.back() == ' ') {
        cleaned_result.pop_back();
    }

    return cleaned_result;
}

// VpsUtils::normalizeVersion
std::string VpsUtils::normalizeVersion(const std::string& version) const {
    std::string normalized = version;

    // Replace commas with dots
    std::replace(normalized.begin(), normalized.end(), ',', '.');

    // Trim leading and trailing whitespace
    size_t first = normalized.find_first_not_of(" \t\n\r\f\v");
    if (std::string::npos == first) {
        return ""; // String is all whitespace
    }
    size_t last = normalized.find_last_not_of(" \t\n\r\f\v");
    normalized = normalized.substr(first, (last - first + 1));

    // Extract the first part before a dash if it matches a numeric pattern
    size_t dash_pos = normalized.find('-');
    if (dash_pos != std::string::npos) {
        std::string first_part = normalized.substr(0, dash_pos);
        // Use a more robust regex to check for numeric pattern (digits and dots)
        if (std::regex_match(first_part, std::regex(R"(\d+(\.\d+)*)"))) {
            return first_part;
        }
    }
    return normalized;
}

// VpsUtils::isVersionGreaterThan
bool VpsUtils::isVersionGreaterThan(const std::string& v1, const std::string& v2) const {
    std::string norm_v1 = normalizeVersion(v1);
    std::string norm_v2 = normalizeVersion(v2);

    if (norm_v1.empty()) return false;
    if (norm_v2.empty()) return true;

    // Split versions into components (e.g., "1.2.3" -> {"1", "2", "3"})
    auto split_version = [](const std::string& version_str) {
        std::vector<std::string> components;
        std::string component;
        std::istringstream ss(version_str);
        while (std::getline(ss, component, '.')) {
            components.push_back(component);
        }
        return components;
    };

    std::vector<std::string> components1 = split_version(norm_v1);
    std::vector<std::string> components2 = split_version(norm_v2);

    size_t max_len = std::max(components1.size(), components2.size());

    for (size_t i = 0; i < max_len; ++i) {
        // Get numeric value for each component, defaulting to 0 if component is missing or not a number
        long long val1 = 0;
        if (i < components1.size()) {
            try {
                val1 = std::stoll(components1[i]);
            } catch (const std::exception&) {
                // If not purely numeric, treat as 0 for numeric parts, then fall back to string comparison
            }
        }

        long long val2 = 0;
        if (i < components2.size()) {
            try {
                val2 = std::stoll(components2[i]);
            } catch (const std::exception&) {
                // Same here
            }
        }

        // Compare numeric parts
        if (val1 > val2) {
            return true;
        }
        if (val1 < val2) {
            return false;
        }

        // If numeric parts are equal, compare original string components if they are not purely numeric
        if (i < components1.size() && i < components2.size()) {
            // Only compare if they are not purely numeric (e.g., "alpha" vs "beta")
            bool is_num1 = std::all_of(components1[i].begin(), components1[i].end(), ::isdigit);
            bool is_num2 = std::all_of(components2[i].begin(), components2[i].end(), ::isdigit);

            if (!is_num1 || !is_num2) { // At least one is non-numeric, do string compare
                int cmp = components1[i].compare(components2[i]);
                if (cmp > 0) return true;
                if (cmp < 0) return false;
            }
        }
    }

    // If all components are equal, versions are the same
    return false;
}

// VpsUtils::extractYearFromDate
std::string VpsUtils::extractYearFromDate(const std::string& dateString) const {
    if (dateString.empty()) {
        return "";
    }

    // Step 1: Trim whitespace
    std::string normalized = dateString;
    size_t first = normalized.find_first_not_of(" \t\n\r\f\v");
    if (std::string::npos == first) {
        return ""; // String is all whitespace
    }
    size_t last = normalized.find_last_not_of(" \t\n\r\f\v");
    normalized = normalized.substr(first, (last - first + 1));

    // Step 2: Normalize separators (replace commas, slashes, hyphens with dots)
    std::replace_if(normalized.begin(), normalized.end(),
        [](char c) { return c == ',' || c == '/' || c == '-'; }, '.');

    // Step 3: Match common date formats with 4-digit year
    // DD.MM.YYYY or D.M.YYYY (where D/M can be 1 or 2 digits)
    std::regex dateRegex_DDMMYYYY(R"(\b\d{1,2}\.\d{1,2}\.(\d{4})\b)");
    // YYYY.MM.DD
    std::regex dateRegex_YYYYMMDD(R"(\b(\d{4})\.\d{1,2}\.\d{1,2}\b)");
    std::smatch match;

    if (std::regex_search(normalized, match, dateRegex_DDMMYYYY) && match.size() > 3) {
        return match[3].str(); // Extract year from DD.MM.YYYY
    }
    if (std::regex_search(normalized, match, dateRegex_YYYYMMDD) && match.size() > 1) {
        return match[1].str(); // Extract year from YYYY.MM.DD
    }

    // Step 4: Match standalone 4-digit year (1900–2100)
    std::regex yearRegex(R"(\b(19\d{2}|20\d{2}|2100)\b)"); // Adjusted to include 2100
    if (std::regex_search(normalized, match, yearRegex) && match.size() > 1) {
        return match[1].str();
    }

    // Step 5: Match short date formats with 2-digit year (e.g., DD.MM.YY)
    // This regex looks for DD.MM.YY or D.M.YY, extracting the last two digits.
    std::regex dateRegex_DDMMYY(R"(\b\d{1,2}\.\d{1,2}\.(\d{2})\b)");
    if (std::regex_search(normalized, match, dateRegex_DDMMYY) && match.size() > 3) {
        std::string yy = match[3].str();
        try {
            int year = std::stoi(yy);
            // Heuristic: 00-49 -> 20xx, 50-99 -> 19xx. This is a common interpretation.
            return (year <= 49 ? "20" : "19") + yy;
        } catch (const std::exception&) {
            LOG_DEBUG("VpsUtils: Failed to convert 2-digit year '" << yy << "' to int.");
        }
    }

    // Step 6: Fallback - search for any 4-digit number that looks like a year (1900-2100 range)
    // This is a less strict check and should be a last resort.
    std::regex fallbackYearRegex(R"((19\d{2}|20\d{2}|2100))");
    if (std::regex_search(normalized, match, fallbackYearRegex) && match.size() > 1) {
        return match[1].str();
    }

    // Step 7: No valid year found
    LOG_DEBUG("VpsUtils: No year found in date string: '" << dateString << "'");
    return "";
}

// VpsUtils::join
std::string VpsUtils::join(const nlohmann::json& array, const std::string& delimiter) const {
    std::vector<std::string> items;
    for (const auto& item : array) {
        try {
            if (item.is_string()) {
                items.push_back(item.get<std::string>());
            } else {
                LOG_DEBUG("VpsUtils: Skipping non-string item in JSON array during join. Type: " << item.type_name());
            }
        } catch (const nlohmann::json::exception& e) {
            LOG_DEBUG("VpsUtils: JSON error skipping invalid array item in join: " << e.what());
        } catch (const std::exception& e) {
            LOG_DEBUG("VpsUtils: General error skipping invalid array item in join: " << e.what());
        }
    }

    // Use accumulate to join strings with delimiter, starting with the first item
    if (items.empty()) {
        return "";
    } else {
        return std::accumulate(std::next(items.begin()), items.end(), items[0],
            [&delimiter](const std::string& a, const std::string& b) { return a + delimiter + b; });
    }
}

// VpsUtils::safeGetString
std::string VpsUtils::safeGetString(const nlohmann::json& j, const std::string& key, const std::string& defaultValue) const {
    if (j.contains(key) && j[key].is_string()) {
        return j[key].get<std::string>();
    } else if (j.contains(key) && j[key].is_null()) {
        return defaultValue; // Explicitly handle null as default
    }
    return defaultValue;
}

// VpsUtils::cleanString
std::string VpsUtils::cleanString(const std::string& input) const {
    std::string result = input;
    // Trim leading whitespace
    size_t first = result.find_first_not_of(" \t\n\r\f\v");
    if (std::string::npos == first) {
        return ""; // String is all whitespace
    }
    result.erase(0, first);

    // Trim trailing whitespace
    size_t last = result.find_last_not_of(" \t\n\r\f\v");
    result.erase(last + 1);

    // Replace multiple spaces with a single space
    std::string cleaned_result;
    bool last_char_was_space = false;
    for (char c : result) {
        if (std::isspace(static_cast<unsigned char>(c))) {
            if (!last_char_was_space) {
                cleaned_result += ' ';
                last_char_was_space = true;
            }
        } else {
            cleaned_result += c;
            last_char_was_space = false;
        }
    }
    return cleaned_result;
}

size_t VpsUtils::levenshteinDistance(const std::string& s1, const std::string& s2) const {
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

std::string VpsUtils::toLower(const std::string& str) const {
    std::string lowerStr = str;
    std::transform(lowerStr.begin(), lowerStr.end(), lowerStr.begin(),
                    [](unsigned char c){ return std::tolower(c); });
    return lowerStr;
}

std::string VpsUtils::extractCleanTitle(const std::string& input) const {
    std::string cleaned = input;

    // 1. Normalize common separators to spaces
    cleaned = std::regex_replace(cleaned, std::regex(R"([_\.])"), " "); // Replace underscores and dots with spaces

    // 2. Remove specific known patterns, case-insensitive. Order matters for regexes.
    // More specific/structured patterns first.
    // Using `(?=\s*$|\s*[\[\(])` for lookahead to ensure it's at the end or followed by a bracket.
    // Using `$` for end-of-string specific patterns.

    std::vector<std::pair<std::regex, std::string>> patterns = {
        // (Manufacturer Year) or (Manufacturer) type patterns at the end, potentially followed by other brackets
        // Example: "Table Name (Manufacturer 1999)" or "Table Name (Manufacturer)"
        {std::regex(R"(\s*\(?[A-Za-z0-9\s&!+-]+\s+\d{4}\)?(?=\s*$|\s*[\[\(]))", std::regex_constants::icase), ""},
        {std::regex(R"(\s*\(?[A-Za-z0-9\s&!+-]+\)(?=\s*$|\s*[\[\(]))", std::regex_constants::icase), ""},

        // Common version indicators at the very end
        // Example: "Table Name v1.0", "Table Name V2", "Table Name 1.0.1"
        {std::regex(R"(\s+v?\d+(\.\d+){0,3}\s*$)"), ""},
        {std::regex(R"(\s+\d+\.\d+\s*$)"), ""}, // Pure decimal versions

        // Common descriptive words with optional year at the end, inside parentheses or not
        // Example: "Table Name (Mod 2020)", "Table Name Mod", "Table Name Recreation 2024"
        {std::regex(R"(\s+\(?[Rr]emake\s*\d{4}\)?$)"), ""},
        {std::regex(R"(\s+\(?[Rr]emastered\s*\d{4}\)?$)"), ""},
        {std::regex(R"(\s+\(?[Mm]od\s*\d{4}\)?$)"), ""},
        {std::regex(R"(\s+\(?[Rr]eskin\s*\d{4}\)?$)"), ""},
        {std::regex(R"(\s+\(?[Rr]ecreation\s*\d{4}\)?$)"), ""},
        {std::regex(R"(\s+\(?[Oo]riginal\s*\d{4}\)?$)"), ""},
        {std::regex(R"(\s+\(?[Hh]omebrew\s*\d{4}\)?$)"), ""},
        {std::regex(R"(\s+\(?[Tt]est\s*\d{4}\)?$)"), ""},

        // Same descriptive words without a year, followed by end of string or other bracket
        {std::regex(R"(\s+\(?[Rr]emake\)?(?=\s*$|\s*[\[\(]))", std::regex_constants::icase), ""},
        {std::regex(R"(\s+\(?[Rr]emastered\)?(?=\s*$|\s*[\[\(]))", std::regex_constants::icase), ""},
        {std::regex(R"(\s+\(?[Mm]od\)?(?=\s*$|\s*[\[\(]))", std::regex_constants::icase), ""},
        {std::regex(R"(\s+\(?[Rr]eskin\)?(?=\s*$|\s*[\[\(]))", std::regex_constants::icase), ""},
        {std::regex(R"(\s+\(?[Rr]ecreation\)?(?=\s*$|\s*[\[\(]))", std::regex_constants::icase), ""},
        {std::regex(R"(\s+\(?[Oo]riginal\)?(?=\s*$|\s*[\[\(]))", std::regex_constants::icase), ""},
        {std::regex(R"(\s+\(?[Hh]omebrew\)?(?=\s*$|\s*[\[\(]))", std::regex_constants::icase), ""},
        {std::regex(R"(\s+\(?[Tt]est\)?(?=\s*$|\s*[\[\(]))", std::regex_constants::icase), ""},
        
        // Author/group indicators (should be somewhat generic to avoid false positives)
        // Example: "Table Name by AuthorName", "Table Name (AuthorName)"
        {std::regex(R"(\s+by\s+[A-Za-z0-9\s&\-]+$)"), ""},
        {std::regex(R"(\s*\(\s*[A-Za-z0-9\s&\-]+\s*\)$)"), ""},
        {std::regex(R"(\s*\[\s*[A-Za-z0-9\s&\-]+\s*\]$)"), ""},

        // Standalone years or series numbers in parentheses/brackets at the very end
        // Example: "Table Name (1999)", "Table Name [2020]"
        {std::regex(R"(\s*\((\d{4}|\d{2})\)$)"), ""},
        {std::regex(R"(\s*\[(\d{4}|\d{2})\]$)"), ""},

        // Any trailing content in parentheses/brackets that wasn't caught (last resort clean-up)
        // This acts as a general cleaner for remaining ( ) or [ ] blocks at the very end.
        {std::regex(R"(\s*[\(\[][^\]\)]*[\)\]]$)"), ""}
    };

    for (const auto& pair : patterns) {
        cleaned = std::regex_replace(cleaned, pair.first, pair.second);
    }

    // 3. Remove any remaining leading/trailing spaces, hyphens, or underscores
    cleaned = std::regex_replace(cleaned, std::regex(R"(^\s*[\s\-\_]*|[\s\-\_]*\s*$)"), "");

    // 4. Collapse multiple spaces into a single space
    cleaned = std::regex_replace(cleaned, std::regex(R"(\s+)"), " ");

    return cleaned;
}

