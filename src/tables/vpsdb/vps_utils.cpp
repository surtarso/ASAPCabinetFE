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
    return items.empty() ? "" : std::accumulate(std::next(items.begin()), items.end(), items[0],
        [&delimiter](const std::string& a, const std::string& b) { return a + delimiter + b; });
}

std::string VpsUtils::normalizeString(const std::string& input) const {
    std::string result = input;
    std::transform(result.begin(), result.end(), result.begin(), ::tolower);
    result.erase(std::remove_if(result.begin(), result.end(), [](char c) {
        return !std::isalnum(c);
    }), result.end());
    return result;
}

std::string VpsUtils::normalizeStringLessAggressive(const std::string& input) const {
    std::string result = input;
    std::transform(result.begin(), result.end(), result.begin(), ::tolower);
    // Remove only specific punctuation, preserve spaces and parentheses
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
    std::replace(normalized.begin(), normalized.end(), ',', '.');
    normalized.erase(0, normalized.find_first_not_of(" \t\n\r\f\v"));
    normalized.erase(normalized.find_last_not_of(" \t\n\r\f\v") + 1);

    size_t dash_pos = normalized.find('-');
    if (dash_pos != std::string::npos) {
        std::string first_part = normalized.substr(0, dash_pos);
        if (std::regex_match(first_part, std::regex("[0-9\\.]+"))) {
            return first_part;
        }
    }
    return normalized;
}

bool VpsUtils::isVersionGreaterThan(const std::string& v1, const std::string& v2) const {
    std::string norm_v1 = normalizeVersion(v1);
    std::string norm_v2 = normalizeVersion(v2);

    if (norm_v1.empty()) return false;
    if (norm_v2.empty()) return true;

    try {
        float f1 = std::stof(norm_v1);
        float f2 = std::stof(norm_v2);
        return f1 > f2;
    } catch (const std::exception&) {
        return norm_v1 > norm_v2;
    }
}

std::string VpsUtils::extractYearFromDate(const std::string& dateString) const {
    std::regex dateRegex_DDMMYYYY("\\d{2}\\.\\d{2}\\.(\\d{4})");
    std::regex dateRegex_YYYY("\\d{4}");
    std::smatch match;

    if (std::regex_search(dateString, match, dateRegex_DDMMYYYY) && match.size() > 1) {
        return match[1].str();
    } else if (std::regex_search(dateString, match, dateRegex_YYYY) && match.size() > 0) {
        return match[0].str();
    }
    return "";
}