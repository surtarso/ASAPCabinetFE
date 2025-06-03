#ifndef VPS_UTILS_H
#define VPS_UTILS_H

#include <string>
#include <json.hpp>

class VpsUtils {
public:
    std::string normalizeString(const std::string& input) const;
    std::string normalizeStringLessAggressive(const std::string& input) const;
    std::string normalizeVersion(const std::string& version) const;
    bool isVersionGreaterThan(const std::string& v1, const std::string& v2) const;
    std::string extractYearFromDate(const std::string& dateString) const;
    std::string join(const nlohmann::json& array, const std::string& delimiter) const;
};

#endif