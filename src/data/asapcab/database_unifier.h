#pragma once

#include <nlohmann/json.hpp>
#include <unordered_map>
#include <unordered_set>
#include <string>

using json = nlohmann::json;

namespace data::asapcabdb {

// Utility to safely extract int from JSON
int safeGetInt(const json& obj, const std::string& key);

// Extract IPDB ID from ipdbUrl if present
std::string extractIpdbIdFromUrl(const std::string& url);

// Build unified record from VPSDB entry with cross-database matching
class DatabaseUnifier {
public:
    struct UnificationResult {
        json unified;
        std::unordered_set<std::string> matched_ipdb_ids;
        std::unordered_set<std::string> matched_lbdb_ids;
        std::unordered_set<std::string> matched_vpinmdb_ids;
    };

    static UnificationResult unify(
        const json& vps_entry,
        int canonical_counter,
        const std::unordered_map<std::string, json>& ipdb_map,
        const std::unordered_map<std::string, json>& lbdb_map,
        const std::unordered_map<std::string, json>& vpinmdb_map);
};

} // namespace data::asapcabdb
