#include "database_unifier.h"
#include "database_matchmaking.h"
#include <string>
// needed for std::set and std::function used when collecting VPIN media URLs
#include <set>
#include <functional>

namespace data::asapcabdb {

int safeGetInt(const json& obj, const std::string& key) {
    if (!obj.contains(key)) return 0;
    try {
        if (obj[key].is_number()) {
            return obj[key].get<int>();
        } else if (obj[key].is_string()) {
            std::string str = obj[key].get<std::string>();
            if (str.empty()) return 0;
            return std::stoi(str);
        }
    } catch (...) {}
    return 0;
}

std::string extractIpdbIdFromUrl(const std::string& url) {
    if (url.empty()) return "";
    size_t pos = url.find("id=");
    if (pos == std::string::npos) return "";
    std::string id = url.substr(pos + 3);
    size_t amp = id.find('&');
    if (amp != std::string::npos) id = id.substr(0, amp);
    return id;
}

DatabaseUnifier::UnificationResult DatabaseUnifier::unify(
    const json& vps_entry,
    int canonical_counter,
    const std::unordered_map<std::string, json>& ipdb_map,
    const std::unordered_map<std::string, json>& lbdb_map,
    const std::unordered_map<std::string, json>& vpinmdb_map) {

    UnificationResult result;
    result.unified = json::object();

    try {
        std::string vps_id = vps_entry.value("id", "");
        result.unified["canonical_id"] = "asapID_" + std::to_string(canonical_counter);
        result.unified["db_sources"] = json::object();
        if (!vps_id.empty()) result.unified["db_sources"]["vpsdb"] = vps_id;

        result.unified["raw_metadata"] = json::object();
        result.unified["raw_metadata"]["vpsdb"] = vps_entry;
        result.unified["vpsdb_all_fields"] = vps_entry;

        result.unified["aliases"] = json::array();
        result.unified["manufacturers"] = json::array();
        result.unified["years"] = json::array();
        result.unified["themes"] = json::array();
        result.unified["images"] = json::array();
        result.unified["links"] = json::array();
        result.unified["roms"] = json::array();
        result.unified["authors"] = json::array();
        result.unified["playerCounts"] = json::array();
        result.unified["tableTypes"] = json::array();
        result.unified["versions"] = json::array();

        // Extract canonical fields from VPSDB
        std::string name = vps_entry.value("name", "");
        std::string manuf = vps_entry.value("manufacturer", "");
        int year = safeGetInt(vps_entry, "year");

        if (!name.empty()) result.unified["canonical_name"] = name;
        if (!manuf.empty()) result.unified["manufacturers"].push_back(manuf);
        if (year != 0) result.unified["years"].push_back(year);

        if (vps_entry.contains("theme") && vps_entry["theme"].is_array()) {
            for (const auto& t : vps_entry["theme"])
                if (t.is_string()) result.unified["themes"].push_back(t.get<std::string>());
        }

        if (vps_entry.contains("author") && vps_entry["author"].is_string()) {
            result.unified["authors"].push_back(vps_entry["author"].get<std::string>());
        }

        int pc = safeGetInt(vps_entry, "playerCount");
        if (pc != 0) result.unified["playerCounts"].push_back(pc);

        if (vps_entry.contains("tableType"))
            result.unified["tableTypes"].push_back(vps_entry["tableType"]);
        if (vps_entry.contains("version"))
            result.unified["versions"].push_back(vps_entry["version"]);

        // Prepare candidate names for matching
        CandidateNames candidates;
        candidates.add(name);
        if (vps_entry.contains("title") && vps_entry["title"].is_string())
            candidates.add(vps_entry["title"].get<std::string>());

        // ---------- IPDB matching ----------
        std::string ipdb_id;
        if (vps_entry.contains("ipdbUrl") && vps_entry["ipdbUrl"].is_string()) {
            ipdb_id = extractIpdbIdFromUrl(vps_entry["ipdbUrl"].get<std::string>());
        }

        if (ipdb_id.empty()) {
            double best_score = 0.0;
            std::string best_id;
            for (const auto& [id, ip_entry] : ipdb_map) {
                try {
                    // be explicit about db types for field mapping; lowered threshold to 0.60
                    MatchScore ms = TableMatcher::scoreMatch(vps_entry, ip_entry, candidates,
                                                             TableMatcher::DatabaseSource::VPSDB,
                                                             TableMatcher::DatabaseSource::IPDB);
                    if (ms.totalScore > best_score && ms.totalScore >= 0.60) {
                        best_score = ms.totalScore;
                        best_id = id;
                    }
                } catch (...) {
                    continue;
                }
            }
            if (!best_id.empty()) ipdb_id = best_id;
        }

        if (!ipdb_id.empty() && ipdb_map.count(ipdb_id)) {
            auto it = ipdb_map.find(ipdb_id);
            result.matched_ipdb_ids.insert(ipdb_id);
            result.unified["db_sources"]["ipdb"] = ipdb_id;
            result.unified["raw_metadata"]["ipdb"] = it->second;
            result.unified["ipdb_all_fields"] = it->second;

            std::string ip_title = it->second.value("Title", "");
            if (!ip_title.empty() && ip_title != name)
                result.unified["aliases"].push_back(ip_title);

            std::string ip_manuf = it->second.value("ManufacturerShortName", "");
            if (!ip_manuf.empty())
                result.unified["manufacturers"].push_back(ip_manuf);

            int ip_year = extractYear(it->second.value("DateOfManufacture", ""));
            if (ip_year != 0) result.unified["years"].push_back(ip_year);

            std::string ip_theme = it->second.value("Theme", "");
            if (!ip_theme.empty()) result.unified["themes"].push_back(ip_theme);

            // Extract images from IPDB ImageFiles[] -> Url
            try {
                if (it->second.contains("ImageFiles") && it->second["ImageFiles"].is_array()) {
                    for (const auto& imgObj : it->second["ImageFiles"]) {
                        if (imgObj.is_object()) {
                            if (imgObj.contains("Url") && imgObj["Url"].is_string()) {
                                result.unified["images"].push_back(imgObj["Url"].get<std::string>());
                            } else if (imgObj.contains("URL") && imgObj["URL"].is_string()) {
                                result.unified["images"].push_back(imgObj["URL"].get<std::string>());
                            }
                        }
                    }
                }
                // Some IPDB exports might have top-level Image or Images fields
                if (it->second.contains("Image") && it->second["Image"].is_string()) {
                    result.unified["images"].push_back(it->second["Image"].get<std::string>());
                }
                if (it->second.contains("Images") && it->second["Images"].is_array()) {
                    for (const auto& im : it->second["Images"]) if (im.is_string()) result.unified["images"].push_back(im.get<std::string>());
                }
            } catch (...) {}

        }

        // ---------- LBDB matching ----------
        std::string lb_best_id;
        {
            double best_score = 0.0;
            std::string best_id;
            for (const auto& [lbid, lb_entry] : lbdb_map) {
                try {
                    MatchScore ms = TableMatcher::scoreMatch(vps_entry, lb_entry, candidates,
                                                             TableMatcher::DatabaseSource::VPSDB,
                                                             TableMatcher::DatabaseSource::LBDB);
                    if (ms.totalScore > best_score && ms.totalScore >= 0.65) {
                        best_score = ms.totalScore;
                        best_id = lbid;
                    }
                } catch (...) {
                    continue;
                }
            }
            if (!best_id.empty()) lb_best_id = best_id;
        }

        if (!lb_best_id.empty() && lbdb_map.count(lb_best_id)) {
            auto it = lbdb_map.find(lb_best_id);
            result.matched_lbdb_ids.insert(lb_best_id);
            result.unified["db_sources"]["lbdb"] = lb_best_id;
            result.unified["raw_metadata"]["lbdb"] = it->second;
            result.unified["lbdb_all_fields"] = it->second;

            std::string lb_title = it->second.value("Name", "");
            if (!lb_title.empty() && lb_title != name)
                result.unified["aliases"].push_back(lb_title);

            // Try to extract images from LBDB common fields
            try {
                if (it->second.contains("Images") && it->second["Images"].is_array()) {
                    for (const auto& im : it->second["Images"]) {
                        if (im.is_string()) result.unified["images"].push_back(im.get<std::string>());
                        else if (im.is_object() && im.contains("Url") && im["Url"].is_string()) result.unified["images"].push_back(im["Url"].get<std::string>());
                    }
                } else if (it->second.contains("Image") && it->second["Image"].is_string()) {
                    result.unified["images"].push_back(it->second["Image"].get<std::string>());
                }
            } catch (...) {}

        }

        // ---------- VPINMDB matching ----------
        // VPINMDB is keyed by VPS id and primarily contains media. Do a direct lookup and attach only media.
        if (!vps_id.empty()) {
            auto vit = vpinmdb_map.find(vps_id);
            if (vit != vpinmdb_map.end()) {
                const json& vpentry = vit->second;
                result.matched_vpinmdb_ids.insert(vps_id);
                result.unified["db_sources"]["vpinmdb"] = vps_id;
                result.unified["raw_metadata"]["vpinmdb"] = vpentry;
                result.unified["vpinmdb_all_fields"] = vpentry;

                // Attach the full VPIN entry under a media key so callers can access structured media directly
                try {
                    result.unified["vpinmdb_media"] = vpentry;
                } catch (...) {}

                // Collect nested URLs from the VPIN entry into unified["images"] (media-only)
                try {
                    std::set<std::string> collected;
                    std::function<void(const json&)> collectUrls = [&](const json& node) {
                        if (node.is_string()) {
                            std::string s = node.get<std::string>();
                            if (s.rfind("http://", 0) == 0 || s.rfind("https://", 0) == 0) collected.insert(s);
                        } else if (node.is_array()) {
                            for (const auto& e : node) collectUrls(e);
                        } else if (node.is_object()) {
                            for (auto it = node.begin(); it != node.end(); ++it) collectUrls(it.value());
                        }
                    };
                    collectUrls(vpentry);
                    for (const auto& url : collected) result.unified["images"].push_back(url);
                } catch (...) {}
            }
            // no multi-factor fallback: VPINMDB entries are 1:1 keyed by VPS id
        }

    } catch (...) {
        // Partial result acceptable
    }

    return result;
}

} // namespace data::asapcabdb
