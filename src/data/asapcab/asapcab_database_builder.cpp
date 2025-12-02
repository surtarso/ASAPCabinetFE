// src/data/asapcab/asapcab_database_builder.cpp
#include "asapcab_database_builder.h"
#include "database_matchmaking.h"
#include "database_unifier.h"
#include "log/logging.h"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
// added for multithreading and clustering
#include <thread>
#include <mutex>
#include <atomic>
#include <cmath>
#include <map>
#include <set>
#include <functional>

using json = nlohmann::json;

namespace data::asapcabdb {

json load_json(const std::string& path) {
    std::ifstream f(path);
    if (!f) return json::object();
    json j;
    f >> j;
    return j;
}

void save_json(const std::string& path, const json& j) {
    std::ofstream f(path);
    f << j.dump(2);
}

json AsapCabDatabaseBuilder::build(const json& db_vpsdb,
                                   const json& db_lbdb,
                                   const json& db_vpinmdb,
                                   const json& db_ipdb) {
    LOG_INFO("Assembling master ASAPCab DB with multi-factor matching...");

    json master;
    master["source_version"] = {
        {"vpsdb", "unknown"},
        {"lbdb", "unknown"},
        {"vpinmdb", "unknown"},
        {"ipdb", "unknown"}
    };

    json tables = json::array();
    std::unordered_set<std::string> matched_ipdb;
    std::unordered_set<std::string> matched_lbdb;
    std::unordered_set<std::string> matched_vpinmdb;

    // Build maps with error handling
    std::unordered_map<std::string, json> ipdb_map;
    for (const auto& entry : db_ipdb) {
        try {
            std::string id = std::to_string(entry.value("IpdbId", 0));
            ipdb_map[id] = entry;
        } catch (...) {
            continue;
        }
    }

    std::unordered_map<std::string, json> lbdb_map;
    for (const auto& lb_entry : db_lbdb) {
        try {
            std::string lb_id = lb_entry.value("Id", "");
            if (lb_id.empty()) continue;
            lbdb_map[lb_id] = lb_entry;
        } catch (...) {
            continue;
        }
    }

    std::unordered_map<std::string, json> vpinmdb_map;
    for (const auto& vpin_entry : db_vpinmdb) {
        try {
            std::string vpin_id = vpin_entry.value("id", "");
            if (vpin_id.empty()) continue;
            vpinmdb_map[vpin_id] = vpin_entry;
        } catch (...) {
            continue;
        }
    }

    LOG_INFO(std::string("Starting table processing: ") + std::to_string(db_vpsdb.size()) +
             " VPSDB entries");

    int canonical_counter = 0;

    // ----------------------------
    // 1) Pre-join LBDB -> IPDB (cheap blocking + scoring)
    // ----------------------------
    auto makeFingerprint = [&](const std::string& s)->std::string {
        if (s.empty()) return std::string();
        std::string tmp = s;
        std::transform(tmp.begin(), tmp.end(), tmp.begin(), [](unsigned char c){ return static_cast<char>(::tolower(c)); });
        tmp.erase(std::remove_if(tmp.begin(), tmp.end(), [](char c){ return !std::isalnum(static_cast<unsigned char>(c)); }), tmp.end());
        if (tmp.size() > 12) tmp.resize(12);
        return tmp;
    };

    // Build IPDB inverted index by fingerprint
    std::unordered_map<std::string, std::vector<std::string>> ipdb_index;
    for (const auto& [id, entry] : ipdb_map) {
        std::string title = entry.value("Title", "");
        std::string fp = makeFingerprint(title);
        if (!fp.empty()) ipdb_index[fp].push_back(id);
    }

    // For each LB entry attempt to find candidate IPDBs and link if confidently matched
    std::unordered_map<std::string, std::string> lb_to_ip; // lbid -> ipdbid
    std::unordered_map<std::string, std::vector<std::string>> ip_to_lbs;
    for (auto& [lbid, lb_entry] : lbdb_map) {
        try {
            std::string lbname = lb_entry.value("Name", "");
            std::string fp = makeFingerprint(lbname);
            std::set<std::string> candidates;
            if (!fp.empty()) {
                auto it = ipdb_index.find(fp);
                if (it != ipdb_index.end()) {
                    for (const auto& cid : it->second) candidates.insert(cid);
                }
            }
            // fallback: try small scan of all ipdb when no candidates (IPDB ~6k, still ok for few misses)
            if (candidates.empty()) {
                // Try manufacturer-keyed candidates
                std::string lbman = lb_entry.value("Manufacturer", "");
                std::string manfp = makeFingerprint(lbman);
                for (const auto& [iid, ie] : ipdb_map) {
                    if (manfp.empty()) {
                        // limited fallback: only pick a few by same first-char
                        std::string t = ie.value("Title", "");
                        if (!t.empty() && tolower(t[0]) == (lbname.empty() ? '\0' : tolower(lbname[0]))) candidates.insert(iid);
                    } else {
                        std::string ipman = ie.value("ManufacturerShortName", "");
                        if (!ipman.empty() && makeFingerprint(ipman) == manfp) candidates.insert(iid);
                    }
                    if (candidates.size() > 30) break; // keep candidate set bounded
                }
            }

            double best_score = 0.0;
            std::string best_ip;
            TableMatcher::CandidateNames cands;
            cands.add(lbname);
            for (const auto& cid : candidates) {
                try {
                    auto& ip_entry = ipdb_map[cid];
                    MatchScore ms = TableMatcher::scoreMatch(lb_entry, ip_entry, cands,
                                                            TableMatcher::DatabaseSource::LBDB,
                                                            TableMatcher::DatabaseSource::IPDB);
                    if (ms.totalScore > best_score) {
                        best_score = ms.totalScore;
                        best_ip = cid;
                    }
                } catch (...) { continue; }
            }
            // threshold conservative for LB->IP linking
            if (!best_ip.empty() && best_score >= 0.60) {
                lb_to_ip[lbid] = best_ip;
                ip_to_lbs[best_ip].push_back(lbid);
                // write hint into lbdb_map so downstream unifier can use it
                lb_entry["linked_ipdb"] = best_ip;
            }
        } catch (...) { continue; }
    }

    // ----------------------------
    // 2) Multi-threaded processing: build work items with preassigned canonical IDs
    //    (VPIN media already merged into VPS entry copies but not persisting?)
    // ----------------------------
    struct WorkItem { json vps_entry; int canonical_id; };
    std::vector<WorkItem> work_items;
    work_items.reserve(db_vpsdb.size());
    for (const auto& vps_entry_orig : db_vpsdb) {
        try {
            std::string vps_id = vps_entry_orig.value("id", "");
            if (vps_id.empty()) continue;

            // Make a modifiable copy of the VPS entry for merging VPIN media
            json vps_entry = vps_entry_orig;

            // --- existing VPIN merge code (already applied earlier) ---
            auto vit = vpinmdb_map.find(vps_id);
            if (vit != vpinmdb_map.end()) {
                const json& vpin_entry = vit->second;

                // Collect image URLs from common VPINMDB fields
                std::vector<std::string> collected_images;
                try {
                    // common array of image URLs
                    if (vpin_entry.contains("images") && vpin_entry["images"].is_array()) {
                        for (const auto& im : vpin_entry["images"]) if (im.is_string()) collected_images.push_back(im.get<std::string>());
                    }
                    // ImageFiles object/array with Url
                    if (vpin_entry.contains("ImageFiles")) {
                        if (vpin_entry["ImageFiles"].is_array()) {
                            for (const auto& imgObj : vpin_entry["ImageFiles"]) {
                                if (imgObj.is_object()) {
                                    if (imgObj.contains("Url") && imgObj["Url"].is_string())
                                        collected_images.push_back(imgObj["Url"].get<std::string>());
                                }
                            }
                        } else if (vpin_entry["ImageFiles"].is_object()) {
                            for (auto it = vpin_entry["ImageFiles"].begin(); it != vpin_entry["ImageFiles"].end(); ++it) {
                                if (it.value().is_string()) collected_images.push_back(it.value().get<std::string>());
                            }
                        }
                    }
                    // scan for any string-looking URLs in top-level fields (fallback)
                    for (auto it = vpin_entry.begin(); it != vpin_entry.end(); ++it) {
                        if (it.value().is_string()) {
                            std::string v = it.value().get<std::string>();
                            if (v.rfind("http://", 0) == 0 || v.rfind("https://", 0) == 0) collected_images.push_back(v);
                        } else if (it.value().is_array()) {
                            bool allStrings = true;
                            for (const auto& e : it.value()) if (!e.is_string()) { allStrings = false; break; }
                            if (allStrings) {
                                for (const auto& e : it.value()) {
                                    std::string v = e.get<std::string>();
                                    if (v.rfind("http://", 0) == 0 || v.rfind("https://", 0) == 0) collected_images.push_back(v);
                                }
                            }
                        }
                    }
                } catch (...) {}

                // Append collected_images into vps_entry["images"] (ensure array and de-dup)
                try {
                    if (!collected_images.empty()) {
                        if (!vps_entry.contains("images") || !vps_entry["images"].is_array()) vps_entry["images"] = json::array();
                        for (const auto& url : collected_images) {
                            bool exists = false;
                            for (const auto& ex : vps_entry["images"]) if (ex.is_string() && ex.get<std::string>() == url) { exists = true; break; }
                            if (!exists) vps_entry["images"].push_back(url);
                        }
                    }
                } catch (...) {}

                // Copy roms/links/other simple fields if present (append if arrays)
                try {
                    if (vpin_entry.contains("roms")) {
                        if (!vps_entry.contains("roms") || !vps_entry["roms"].is_array()) vps_entry["roms"] = json::array();
                        if (vpin_entry["roms"].is_array()) {
                            for (const auto& r : vpin_entry["roms"]) if (r.is_string()) vps_entry["roms"].push_back(r.get<std::string>());
                        } else if (vpin_entry["roms"].is_string()) {
                            vps_entry["roms"].push_back(vpin_entry["roms"].get<std::string>());
                        }
                    }
                    if (vpin_entry.contains("links")) {
                        if (!vps_entry.contains("links") || !vps_entry["links"].is_array()) vps_entry["links"] = json::array();
                        if (vpin_entry["links"].is_array()) {
                            for (const auto& l : vpin_entry["links"]) if (l.is_string()) vps_entry["links"].push_back(l.get<std::string>());
                        } else if (vpin_entry["links"].is_string()) {
                            vps_entry["links"].push_back(vpin_entry["links"].get<std::string>());
                        }
                    }
                    // copy author/version/tableType if present and not already in VPS entry
                    if (vpin_entry.contains("author") && vpin_entry["author"].is_string()) {
                        if (!vps_entry.contains("author") || (vps_entry["author"].is_string() && vps_entry["author"].get<std::string>().empty()))
                            vps_entry["author"] = vpin_entry["author"].get<std::string>();
                    }
                    if (vpin_entry.contains("version")) {
                        if (!vps_entry.contains("version")) vps_entry["version"] = vpin_entry["version"];
                    }
                    if (vpin_entry.contains("tableType")) {
                        if (!vps_entry.contains("tableType")) vps_entry["tableType"] = vpin_entry["tableType"];
                    }
                    // add a marker for debugging/audit that VPIN media was merged
                    vps_entry["merged_vpin_id"] = vps_id;
                } catch (...) {}
            }
            // --- END new merging ---

            // push work item
            work_items.push_back({vps_entry, ++canonical_counter});
        } catch (...) {
            continue;
        }
    }

    size_t total_items = work_items.size();
    if (total_items == 0) {
        LOG_INFO("No VPSDB entries to process.");
    } else {
        unsigned int hw = std::thread::hardware_concurrency();
        size_t num_threads = (hw == 0) ? 1 : std::max<size_t>(1, (hw * 8) / 10); // 80% of cores

        LOG_INFO(std::string("Spawning ") + std::to_string(num_threads) + " worker threads for " + std::to_string(total_items) + " items");

        std::atomic<size_t> next_index{0};
        std::atomic<size_t> processed_count{0};
        std::mutex result_mutex;
        std::vector<json> unified_results;
        unified_results.reserve(total_items);

        // store per-record match info for union-find clustering
        struct RecordMatch {
            std::string canonical_id;
            std::string vps_id;
            std::vector<std::string> ipdb_ids;
            std::vector<std::string> lbdb_ids;
            std::vector<std::string> vpin_ids;
            json unified;
        };
        std::vector<RecordMatch> match_records;
        match_records.reserve(total_items);

        // thread-local sets merged under result_mutex
        std::unordered_set<std::string> local_matched_ipdb;
        std::unordered_set<std::string> local_matched_lbdb;
        std::unordered_set<std::string> local_matched_vpinmdb;

        // worker with no unused params
        auto worker_fn = [&]() {
            while (true) {
                size_t idx = next_index.fetch_add(1, std::memory_order_relaxed);
                if (idx >= total_items) break;
                const WorkItem item = work_items[idx];
                try {
                    auto unification = DatabaseUnifier::unify(item.vps_entry, item.canonical_id, ipdb_map, lbdb_map, vpinmdb_map);

                    RecordMatch rec;
                    rec.canonical_id = unification.unified.value("canonical_id", std::string("asapID_" + std::to_string(item.canonical_id)));
                    rec.vps_id = item.vps_entry.value("id", "");
                    rec.unified = unification.unified;

                    for (const auto& s : unification.matched_ipdb_ids) rec.ipdb_ids.push_back(s);
                    for (const auto& s : unification.matched_lbdb_ids) rec.lbdb_ids.push_back(s);
                    for (const auto& s : unification.matched_vpinmdb_ids) rec.vpin_ids.push_back(s);

                    {
                        std::lock_guard<std::mutex> lock(result_mutex);
                        unified_results.push_back(unification.unified);
                        match_records.push_back(std::move(rec));
                        local_matched_ipdb.insert(unification.matched_ipdb_ids.begin(), unification.matched_ipdb_ids.end());
                        local_matched_lbdb.insert(unification.matched_lbdb_ids.begin(), unification.matched_lbdb_ids.end());
                        local_matched_vpinmdb.insert(unification.matched_vpinmdb_ids.begin(), unification.matched_vpinmdb_ids.end());
                    }

                    size_t done = processed_count.fetch_add(1, std::memory_order_relaxed) + 1;
                    if ((done % 100) == 0) {
                        LOG_INFO(std::string("Processed ") + std::to_string(done) + " / " + std::to_string(total_items) + " VPSDB tables");
                    }
                } catch (const std::exception& e) {
                    LOG_INFO(std::string("Error processing VPSDB entry in worker: ") + e.what());
                    continue;
                } catch (...) {
                    continue;
                }
            }
        };

        std::vector<std::thread> workers;
        workers.reserve(num_threads);
        for (size_t i = 0; i < num_threads; ++i) workers.emplace_back(worker_fn);
        for (auto& t : workers) t.join();

        // Merge thread-local matched ids into global sets
        matched_ipdb.insert(local_matched_ipdb.begin(), local_matched_ipdb.end());
        matched_lbdb.insert(local_matched_lbdb.begin(), local_matched_lbdb.end());
        matched_vpinmdb.insert(local_matched_vpinmdb.begin(), local_matched_vpinmdb.end());

        // ----------------------------
        // 3) Union-find / transitive clustering across all matched ids
        // ----------------------------
        struct DisjointSet {
            std::unordered_map<std::string, std::string> parent;
            std::string find(const std::string& x) {
                auto it = parent.find(x);
                if (it == parent.end()) { parent[x] = x; return x; }
                if (it->second == x) return x;
                return parent[x] = find(it->second);
            }
            void unite(const std::string& a, const std::string& b) {
                std::string ra = find(a);
                std::string rb = find(b);
                if (ra == rb) return;
                parent[rb] = ra;
            }
        } ds;

        auto makeNode = [](const std::string& prefix, const std::string& id)->std::string {
            return prefix + ":" + id;
        };

        // add unions from match_records
        for (const auto& r : match_records) {
            std::string canonNode = makeNode("canon", r.canonical_id);
            if (!r.vps_id.empty()) ds.unite(canonNode, makeNode("vps", r.vps_id));
            for (const auto& ip : r.ipdb_ids) ds.unite(canonNode, makeNode("ipdb", ip));
            for (const auto& lb : r.lbdb_ids) ds.unite(canonNode, makeNode("lbdb", lb));
            for (const auto& vp : r.vpin_ids) ds.unite(canonNode, makeNode("vpin", vp));
        }

        // Build clusters: root -> list of record indices and source ids
        std::unordered_map<std::string, std::vector<size_t>> cluster_records; // root -> indices into match_records
        std::unordered_map<std::string, std::set<std::string>> cluster_ipdbs, cluster_lbdbs, cluster_vpins, cluster_vps;
        for (size_t i = 0; i < match_records.size(); ++i) {
            const auto& r = match_records[i];
            std::string root = ds.find(makeNode("canon", r.canonical_id));
            cluster_records[root].push_back(i);
            if (!r.vps_id.empty()) cluster_vps[root].insert(r.vps_id);
            for (const auto& ip : r.ipdb_ids) cluster_ipdbs[root].insert(ip);
            for (const auto& lb : r.lbdb_ids) cluster_lbdbs[root].insert(lb);
            for (const auto& vp : r.vpin_ids) cluster_vpins[root].insert(vp);
        }

        // --- NEW: Expand clusters to include LBDB entries that were pre-linked to any IPDBs in the cluster
        for (auto& kv : cluster_records) {
            const std::string& root = kv.first;
            auto& lbs = cluster_lbdbs[root];
            auto& ips = cluster_ipdbs[root];
            for (const auto& ipid : ips) {
                auto it = ip_to_lbs.find(ipid);
                if (it != ip_to_lbs.end()) {
                    for (const auto& lbid : it->second) {
                        lbs.insert(lbid);
                        // Also ensure the lb -> ip link is present in lbdb_map (it was earlier set), but even if not, we add it to cluster
                    }
                }
            }
        }

        // --- NEW: mark all cluster members as matched so they are not emitted as "iso_*" later ---
        for (const auto& kv : cluster_ipdbs) {
            for (const auto& ipid : kv.second) matched_ipdb.insert(ipid);
        }
        for (const auto& kv : cluster_lbdbs) {
            for (const auto& lbid : kv.second) matched_lbdb.insert(lbid);
        }
        for (const auto& kv : cluster_vpins) {
            for (const auto& vpid : kv.second) matched_vpinmdb.insert(vpid);
        }

        // Merge clusters into final canonical tables
        std::vector<json> merged_tables;
        merged_tables.reserve(cluster_records.size());

        for (const auto& [root, rec_idxs] : cluster_records) {
            json merged = json::object();
            // pick a canonical id: use first record's canonical_id
            std::string chosen_canon = match_records[rec_idxs.front()].canonical_id;
            merged["canonical_id"] = chosen_canon;
            merged["db_sources"] = json::object();
            merged["raw_metadata"] = json::object();
            // collector sets for arrays
            std::set<std::string> aliases_set, images_set, roms_set, links_set, authors_set, manufacturers_set, years_set, playerCounts_set, tableTypes_set, versions_set;

            std::string preferred_name;
            // precedence: VPS unified canonical_name, then IPDB Title, then LBDB Name
            for (size_t idx : rec_idxs) {
                const auto& u = match_records[idx].unified;
                if (preferred_name.empty() && u.contains("canonical_name") && u["canonical_name"].is_string()) {
                    preferred_name = u["canonical_name"].get<std::string>();
                }
                // merge db_sources
                if (u.contains("db_sources") && u["db_sources"].is_object()) {
                    for (auto it = u["db_sources"].begin(); it != u["db_sources"].end(); ++it) {
                        merged["db_sources"][it.key()] = it.value();
                    }
                }
                // merge raw_metadata per source
                if (u.contains("raw_metadata") && u["raw_metadata"].is_object()) {
                    for (auto it = u["raw_metadata"].begin(); it != u["raw_metadata"].end(); ++it) {
                        merged["raw_metadata"][it.key()] = it.value();
                    }
                }
                // arrays: aliases, images, roms, links, authors, manufacturers, years, playerCounts, tableTypes, versions
                auto collectArray = [&](const json& obj, const std::string& key, std::set<std::string>& out) {
                    if (!obj.contains(key)) return;
                    if (obj[key].is_array()) {
                        for (const auto& e : obj[key]) if (e.is_string()) out.insert(e.get<std::string>());
                    } else if (obj[key].is_string()) out.insert(obj[key].get<std::string>());
                };
                collectArray(u, "aliases", aliases_set);
                collectArray(u, "images", images_set);
                collectArray(u, "roms", roms_set);
                collectArray(u, "links", links_set);
                collectArray(u, "authors", authors_set);
                collectArray(u, "manufacturers", manufacturers_set);
                // years may be numbers or strings; store as string for de-dupe
                if (u.contains("years") && u["years"].is_array()) {
                    for (const auto& y : u["years"]) {
                        if (y.is_number()) years_set.insert(std::to_string(y.get<int>()));
                        else if (y.is_string()) years_set.insert(y.get<std::string>());
                    }
                }
                if (u.contains("playerCounts") && u["playerCounts"].is_array()) {
                    for (const auto& p : u["playerCounts"]) {
                        if (p.is_number()) playerCounts_set.insert(std::to_string(p.get<int>()));
                        else if (p.is_string()) playerCounts_set.insert(p.get<std::string>());
                    }
                }
                collectArray(u, "tableTypes", tableTypes_set);
                collectArray(u, "versions", versions_set);
            }

            // --- NEW: incorporate IPDB raw entries (if present in cluster_ipdbs but not included already)
            for (const auto& ipid : cluster_ipdbs[root]) {
                try {
                    if (ipdb_map.count(ipid)) {
                        const json& ipentry = ipdb_map[ipid];
                        // add to merged db_sources (support multiple ids)
                        if (!merged["db_sources"].contains("ipdb")) merged["db_sources"]["ipdb"] = json::array();
                        if (merged["db_sources"]["ipdb"].is_array()) merged["db_sources"]["ipdb"].push_back(ipid);
                        // add raw metadata entries (array)
                        if (!merged["raw_metadata"].contains("ipdb")) merged["raw_metadata"]["ipdb"] = json::array();
                        merged["raw_metadata"]["ipdb"].push_back(ipentry);

                        // extract images/manufacturer/name/years from ipentry
                        try {
                            // ImageFiles -> Url
                            if (ipentry.contains("ImageFiles") && ipentry["ImageFiles"].is_array()) {
                                for (const auto& imgObj : ipentry["ImageFiles"]) {
                                    if (imgObj.is_object()) {
                                        if (imgObj.contains("Url") && imgObj["Url"].is_string()) images_set.insert(imgObj["Url"].get<std::string>());
                                    }
                                }
                            }
                            if (ipentry.contains("Images") && ipentry["Images"].is_array()) {
                                for (const auto& im : ipentry["Images"]) if (im.is_string()) images_set.insert(im.get<std::string>());
                            }
                            if (ipentry.contains("Image") && ipentry["Image"].is_string()) images_set.insert(ipentry["Image"].get<std::string>());

                            std::string ipman = ipentry.value("ManufacturerShortName", "");
                            if (!ipman.empty()) manufacturers_set.insert(ipman);
                            std::string iptitle = ipentry.value("Title", "");
                            if (!iptitle.empty()) aliases_set.insert(iptitle);

                            int ipy = extractYear(ipentry.value("DateOfManufacture", ""));
                            if (ipy != 0) years_set.insert(std::to_string(ipy));
                        } catch (...) {}
                    }
                } catch (...) {}
            }

            // --- NEW: incorporate LBDB raw entries (from cluster_lbdbs)
            for (const auto& lbid : cluster_lbdbs[root]) {
                try {
                    if (lbdb_map.count(lbid)) {
                        const json& lbentry = lbdb_map[lbid];

                        // Helper: convert LBDB image filename to full LaunchBox URL when needed
                        auto normalizeLbImage = [](const std::string& s)->std::string {
                            if (s.empty()) return s;
                            const std::string launchboxPrefix = "https://images.launchbox-app.com/";
                            if (s.rfind("http://", 0) == 0 || s.rfind("https://", 0) == 0) return s;
                            if (s.rfind(launchboxPrefix, 0) == 0) return s;
                            return launchboxPrefix + s;
                        };

                        // add to merged db_sources (support multiple ids)
                        if (!merged["db_sources"].contains("lbdb")) merged["db_sources"]["lbdb"] = json::array();
                        if (merged["db_sources"]["lbdb"].is_array()) merged["db_sources"]["lbdb"].push_back(lbid);
                        // add raw metadata entries (array)
                        if (!merged["raw_metadata"].contains("lbdb")) merged["raw_metadata"]["lbdb"] = json::array();
                        merged["raw_metadata"]["lbdb"].push_back(lbentry);

                        // extract images/manufacturer/name/years from lbentry
                        try {
                            // LBDB might store images under various keys: "images", "Images", "images" object etc.
                            if (lbentry.contains("images") && lbentry["images"].is_array()) {
                                for (const auto& im : lbentry["images"]) if (im.is_string()) images_set.insert(normalizeLbImage(im.get<std::string>()));
                            } else if (lbentry.contains("images") && lbentry["images"].is_object()) {
                                for (auto it = lbentry["images"].begin(); it != lbentry["images"].end(); ++it) {
                                    // values may be arrays of filenames
                                    if (it.value().is_array()) {
                                        for (const auto& v : it.value()) if (v.is_string()) images_set.insert(normalizeLbImage(v.get<std::string>()));
                                    } else if (it.value().is_string()) images_set.insert(normalizeLbImage(it.value().get<std::string>()));
                                }
                            }
                            if (lbentry.contains("Images") && lbentry["Images"].is_array()) {
                                for (const auto& im : lbentry["Images"]) if (im.is_string()) images_set.insert(normalizeLbImage(im.get<std::string>()));
                            }
                            if (lbentry.contains("Image") && lbentry["Image"].is_string()) images_set.insert(normalizeLbImage(lbentry["Image"].get<std::string>()));

                            std::string lbman = lbentry.value("Manufacturer", "");
                            if (!lbman.empty()) manufacturers_set.insert(lbman);
                            std::string lbtitle = lbentry.value("Name", "");
                            if (!lbtitle.empty()) aliases_set.insert(lbtitle);

                            // year may be string
                            std::string lby = lbentry.value("Year", "");
                            if (!lby.empty()) years_set.insert(lby);
                        } catch (...) {}
                    }
                } catch (...) {}
            }

            // NOT WORKING: incorporate VPINMDB media structure (from cluster_vpins)
            // Store full nested media by vpsdb id for easy reference/lookup
            // if (!cluster_vpins[root].empty()) {
            //     merged["vpinmdb_media"] = json::object();
            //     for (const auto& vpid : cluster_vpins[root]) {
            //         if (vpinmdb_map.count(vpid)) {
            //             const json& vpentry = vpinmdb_map[vpid];
            //             // Store the entire vpin entry media structure keyed by its vpsdb id
            //             merged["vpinmdb_media"][vpid] = vpentry;
            //         }
            //     }
            // }

            // After including additional sources, set canonical_name if empty and prefer VPS, then IPDB, then LBDB
            if (!preferred_name.empty()) merged["canonical_name"] = preferred_name;
            else {
                // attempt IPDB title
                if (!cluster_ipdbs[root].empty()) {
                    for (const auto& ipid : cluster_ipdbs[root]) {
                        if (ipdb_map.count(ipid)) {
                            std::string t = ipdb_map[ipid].value("Title", "");
                            if (!t.empty()) { merged["canonical_name"] = t; break; }
                        }
                    }
                }
                // fallback to LBDB name
                if (!merged.contains("canonical_name") && !cluster_lbdbs[root].empty()) {
                    for (const auto& lbid : cluster_lbdbs[root]) {
                        if (lbdb_map.count(lbid)) {
                            std::string t = lbdb_map[lbid].value("Name", "");
                            if (!t.empty()) { merged["canonical_name"] = t; break; }
                        }
                    }
                }
            }

            // push arrays from sets
            auto pushSetArray = [&](const std::set<std::string>& s, const std::string& key) {
                merged[key] = json::array();
                for (const auto& v : s) merged[key].push_back(v);
            };
            if (!aliases_set.empty()) pushSetArray(aliases_set, "aliases");
            if (!images_set.empty()) pushSetArray(images_set, "images");
            if (!roms_set.empty()) pushSetArray(roms_set, "roms");
            if (!links_set.empty()) pushSetArray(links_set, "links");
            if (!authors_set.empty()) pushSetArray(authors_set, "authors");
            if (!manufacturers_set.empty()) pushSetArray(manufacturers_set, "manufacturers");
            if (!years_set.empty()) {
                merged["years"] = json::array();
                for (const auto& y : years_set) {
                    try { merged["years"].push_back(std::stoi(y)); } catch (...) { merged["years"].push_back(y); }
                }
            }
            if (!playerCounts_set.empty()) {
                merged["playerCounts"] = json::array();
                for (const auto& p : playerCounts_set) {
                    try { merged["playerCounts"].push_back(std::stoi(p)); } catch (...) { merged["playerCounts"].push_back(p); }
                }
            }
            if (!tableTypes_set.empty()) pushSetArray(tableTypes_set, "tableTypes");
            if (!versions_set.empty()) pushSetArray(versions_set, "versions");

            merged_tables.push_back(std::move(merged));
        }

        // Append merged_tables to final tables
        for (auto& m : merged_tables) tables.push_back(m);
    } // end else(total_items != 0)

    // Add unmatched IPDB as isolated (skip ones already matched via matched_ipdb set)
    for (const auto& [id, entry] : ipdb_map) {
        try {
            if (matched_ipdb.count(id)) continue;
            json isolated = json::object();
            isolated["canonical_id"] = "iso_ipdb_" + id;
            isolated["db_sources"] = {{"ipdb", id}};
            isolated["raw_metadata"] = {{"ipdb", entry}};
            isolated["canonical_name"] = entry.value("Title", "");
            tables.push_back(isolated);
        } catch (...) {
            continue;
        }
    }

    // Add unmatched LBDB
    for (const auto& [id, entry] : lbdb_map) {
        try {
            if (matched_lbdb.count(id)) continue;
            json isolated = json::object();
            isolated["canonical_id"] = "iso_lbdb_" + id;
            isolated["db_sources"] = {{"lbdb", id}};
            isolated["raw_metadata"] = {{"lbdb", entry}};
            isolated["canonical_name"] = entry.value("Name", "");
            tables.push_back(isolated);
        } catch (...) {
            continue;
        }
    }

    // Add unmatched VPINMDB
    for (const auto& [id, entry] : vpinmdb_map) {
        try {
            if (matched_vpinmdb.count(id)) continue;
            json isolated = json::object();
            isolated["canonical_id"] = "iso_vpinmdb_" + id;
            isolated["db_sources"] = {{"vpinmdb", id}};
            isolated["raw_metadata"] = {{"vpinmdb", entry}};
            isolated["canonical_name"] = entry.value("name", "");
            tables.push_back(isolated);
        } catch (...) {
            continue;
        }
    }

    master["tables"] = tables;
    master["raw"] = {
        {"vpsdb", db_vpsdb},
        {"lbdb", db_lbdb},
        {"vpinmdb", db_vpinmdb},
        {"ipdb", db_ipdb}
    };

    LOG_INFO(std::string("Master database assembled: ") + std::to_string(tables.size()) +
             " total tables");
    return master;
}

} // namespace data::asapcabdb
