// src/tables/launchboxdb/lbdb_downloader.cpp
#include "lbdb_scanner.h"
#include "lbdb_image.h"
#include "data/lbdb/lbdb_builder.h"
#include "data/lbdb/lbdb_updater.h"
#include "data/lbdb/lbdb_loader.h"
#include "data/manufacturers.h"
#include "data/vpinmdb/vpinmdb_downloader.h"
#include "utils/string_utils.h"
#include "log/logging.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#include <mutex>
#include <memory>

// TODO: collect ldbdDeveloper ldbdName ldbdPublisher ldbdYear on matches to table_data !
// Developer is a mix of Author and manufacturers... Publisher = Manufacturer
// Remember manufacturers.h list!

namespace fs = std::filesystem;

// ---------------------------------------------------------
// INDEX TYPES & BUILDER (file-scope static helpers)
// ---------------------------------------------------------
struct LBIndex {
    // compact representation for each LB entry we care about
    struct Entry {
        const nlohmann::json* jsonPtr;   // pointer into the loaded DB
        std::string normTitle;           // normalized title (less aggressive)
        std::vector<std::string> tokens; // tokenized normTitle
        std::string year;
        std::string manuNorm;            // normalized manufacturer (canonical from list if possible)
    };

    std::vector<Entry> entries;

    // maps for fast candidate lookup
    std::unordered_map<std::string, std::vector<size_t>> byNormTitle; // exact norm title -> indices
    std::unordered_map<std::string, std::vector<size_t>> byToken;     // token -> indices
    std::unordered_map<std::string, std::vector<size_t>> byYear;      // year -> indices
    std::unordered_map<std::string, std::vector<size_t>> byManufacturer; // manu -> indices

    void clear() {
        entries.clear();
        byNormTitle.clear();
        byToken.clear();
        byYear.clear();
        byManufacturer.clear();
    }
};

static LBIndex s_index;                // single global index for this compilation unit
static bool s_indexBuilt = false;
static std::mutex s_indexMutex;

// Helper: split tokens (simple)
static inline std::vector<std::string> tokenizeSimple(const std::string& s) {
    std::vector<std::string> out;
    std::istringstream iss(s);
    std::string w;
    while (iss >> w) out.push_back(w);
    return out;
}

// Build the index from the in-memory DB (call once)
static void buildLbIndex(const nlohmann::json& db, const StringUtils& util) {
    std::lock_guard<std::mutex> lk(s_indexMutex);
    if (s_indexBuilt) return; // double-check inside lock

    s_index.clear();
    s_index.entries.reserve(db.size());

    size_t idx = 0;
    for (const auto& game : db) {
        LBIndex::Entry e;
        e.jsonPtr = &game;

        // name/year/manu safe extraction
        std::string title = util.safeGetString(game, "Name", "");
        std::string year  = util.safeGetString(game, "Year", "");
        std::string manu  = util.safeGetString(game, "Developer", "");

        std::string cleanB = util.extractCleanTitle(title);
        e.normTitle = util.normalizeStringLessAggressive(cleanB);
        e.tokens = tokenizeSimple(e.normTitle);
        e.year = year;
        std::string manuLower = util.toLower(manu);

        // canonicalize using manufacturers list if possible
        bool manuFound = false;
        for (const auto& m : PinballManufacturers::MANUFACTURERS_LOWERCASE) {
            if (manuLower.find(m) != std::string::npos) {
                e.manuNorm = m;
                manuFound = true;
                break;
            }
        }
        if (!manuFound) e.manuNorm = util.toLower(manu);

        s_index.entries.push_back(std::move(e));

        // populate maps using index 'idx'
        const LBIndex::Entry& entryRef = s_index.entries.back();
        if (!entryRef.normTitle.empty()) {
            s_index.byNormTitle[entryRef.normTitle].push_back(idx);
        }
        for (const auto& t : entryRef.tokens) {
            if (t.size() > 1) s_index.byToken[t].push_back(idx);
        }
        if (!entryRef.year.empty()) s_index.byYear[entryRef.year].push_back(idx);
        if (!entryRef.manuNorm.empty()) s_index.byManufacturer[entryRef.manuNorm].push_back(idx);

        ++idx;
    }

    s_indexBuilt = true;
}


std::optional<std::string> LbdbScanner::findBestMatch(const TableData& table) {
    static StringUtils util;
    static nlohmann::json db;

    // Load DB once if needed (mirror previous behavior)
    if (db.empty()) {
        std::ifstream f(settings_.lbdbPath);
        if (!f.is_open()) return std::nullopt;
        try { f >> db; } catch (...) { return std::nullopt; }
    }

    // Ensure index is built (thread-safe)
    if (!s_indexBuilt) {
        buildLbIndex(db, util);
    }

    // ------------------------------------------
    // CLEAN ASAP TITLE (aggressive, VPS-style)
    // ------------------------------------------
    std::string cleanA = util.extractCleanTitle(table.bestTitle);
    std::string normA  = util.normalizeStringLessAggressive(cleanA);
    auto tokensA = tokenizeSimple(normA);

    // Normalize year and manufacturer
    std::string yearA = table.bestYear;
    std::string manuA = util.toLower(table.bestManufacturer);
    bool manuKnown = false;
    for (const auto& m : PinballManufacturers::MANUFACTURERS_LOWERCASE) {
        if (manuA.find(m) != std::string::npos) {
            manuKnown = true;
            manuA = m;
            break;
        }
    }

    // -----------------------
    // Gather candidate indices
    // -----------------------
    std::unordered_set<size_t> candidates;

    // 1) exact title candidates
    if (!normA.empty()) {
        auto it = s_index.byNormTitle.find(normA);
        if (it != s_index.byNormTitle.end()) {
            for (size_t i : it->second) candidates.insert(i);
        }
    }

    // 2) token candidates (collect from first few tokens; prioritized)
    int tokenCount = 0;
    for (const auto& t : tokensA) {
        if (t.size() <= 2) continue; // skip tiny noise tokens
        auto it = s_index.byToken.find(t);
        if (it != s_index.byToken.end()) {
            for (size_t i : it->second) candidates.insert(i);
        }
        if (++tokenCount >= 6) break; // don't query too many tokens
    }

    // 3) year candidates
    if (!yearA.empty()) {
        auto it = s_index.byYear.find(yearA);
        if (it != s_index.byYear.end()) {
            for (size_t i : it->second) candidates.insert(i);
        }
    }

    // 4) manufacturer candidates
    if (manuKnown) {
        auto it = s_index.byManufacturer.find(manuA);
        if (it != s_index.byManufacturer.end()) {
            for (size_t i : it->second) candidates.insert(i);
        }
    }

    // If no candidates found by index, fall back to a narrow global pass:
    // search by first meaningful token across entire DB (fast compared to brute force)
    if (candidates.empty() && !tokensA.empty()) {
        const std::string& firstTok = tokensA.front();
        if (!firstTok.empty()) {
            auto it = s_index.byToken.find(firstTok);
            if (it != s_index.byToken.end()) {
                for (size_t i : it->second) candidates.insert(i);
            }
        }
    }

    // If still empty, give up (avoid full DB scan)
    if (candidates.empty()) {
        return std::nullopt;
    }

    // -----------------------
    // Score candidates only
    // -----------------------
    int bestScore = 0;
    std::string bestId;

    for (size_t idx : candidates) {
        const auto& e = s_index.entries[idx];
        int score = 0;

        const std::string& normB = e.normTitle;
        const std::vector<std::string>& tokensB = e.tokens;
        const std::string& yearB = e.year;
        const std::string& manuBnorm = e.manuNorm;

        // EXACT TITLE MATCH
        if (!normA.empty() && normA == normB) score += 200;

        // SUBSTRING MATCH
        if (normA.size() > 3 && normB.size() > 3) {
            if (normA.find(normB) != std::string::npos ||
                normB.find(normA) != std::string::npos) score += 120;
        }

        // WORD INTERSECTION
        int commonWords = 0;
        for (const auto& wa : tokensA) {
            for (const auto& wb : tokensB) {
                if (wa == wb && wa.size() > 2) commonWords++;
            }
        }
        score += commonWords * 40;

        // YEAR MATCH
        if (!yearA.empty() && !yearB.empty() && yearA == yearB) score += 70;

        // MANUFACTURER MATCH
        if (manuKnown && !manuBnorm.empty() && manuA == manuBnorm) score += 60;

        if (!manuBnorm.empty()) score += 5;
        if (!yearB.empty()) score += 5;

        if (score > bestScore) {
            bestScore = score;
            // retrieve ID from the json pointer
            if (e.jsonPtr && e.jsonPtr->contains("Id") && (*e.jsonPtr)["Id"].is_string()) {
                bestId = (*e.jsonPtr)["Id"].get<std::string>();
            } else {
                bestId.clear();
            }
        }
    }

    if (bestScore >= 120 && !bestId.empty()) return bestId;
    return std::nullopt;
}


// ------------------------------------------------------
// scanForMedia()
// now using findBestMatch()
// ------------------------------------------------------
void LbdbScanner::scanForMedia(std::vector<TableData>& tables) {

    if (!settings_.downloadFlyersImage && !settings_.downloadTopperLogoImage) {
        LOG_WARN("No LaunchBox media enabled. Skipping LBDB.");
        return;
    }

    // ---------------------------
    // Ensure DB exists (new)
    // ---------------------------
    {
        data::lbdb::LbdbUpdater updater(settings_, progress_);
        if (!updater.ensureAvailable()) {
            LOG_ERROR("LaunchBox DB not available");
            return;
        }
    }

    // ---------------------------
    // Load DB (new)
    // ---------------------------
    static nlohmann::json pinballDb;
    if (pinballDb.empty()) {
        data::lbdb::LbdbLoader loader(settings_, progress_);
        pinballDb = loader.load();
        if (pinballDb.empty()) {
            LOG_ERROR("LaunchBox DB failed to load");
            return;
        }
    }

    // -----------------------------------
    // Build index once (existing behavior)
    // -----------------------------------
    static StringUtils util;
    if (!s_indexBuilt) {
        buildLbIndex(pinballDb, util);
    }

    // Process each table
    size_t processed = 0;

    for (auto& table : tables) {

        // clear previous ID BEFORE matching
        table.lbdbID.clear();

        // ------------------------------------------------------
        // perform robust match using findBestMatch()
        // ------------------------------------------------------
        auto best = findBestMatch(table);
        if (!best.has_value()) {
            table.lbdbID = "";
            LOG_WARN("LaunchBox: NO MATCH → " + table.bestTitle);
        } else {
            std::string bestId = best.value();
            table.lbdbID = bestId; // store clean new ID

            LOG_INFO("LaunchBox MATCH → " + table.bestTitle +
                    " (ID: " + bestId + ")");

            // Download images
            if (settings_.downloadTopperLogoImage)
                downloadClearLogo(bestId, table, pinballDb);
            if (settings_.downloadFlyersImage)
                downloadFlyersFromJson(bestId, table, pinballDb);
        }

        // Update UI progress
        processed++;
        if (progress_) {
            std::lock_guard<std::mutex> l(progress_->mutex);
            progress_->currentTablesLoaded = processed;
            progress_->logMessages.push_back("LaunchBox: " + table.bestTitle);
        }
    }
}

// Classic DMD logo (always just gameId.png)
void LbdbScanner::downloadClearLogo(const std::string& gameId,
                                       TableData& table,
                                       const nlohmann::json& db) {
    auto it = std::find_if(db.begin(), db.end(),
        [&gameId](const auto& g) {
            return g["Id"].template get<std::string>() == gameId;
        });

    if (it == db.end() || !it->contains("images")) {
        LOG_WARN("LaunchBox: no image block for " + table.bestTitle);
        return;
    }

    const auto& images = (*it)["images"];

    if (!images.contains("Clear Logo") || images["Clear Logo"].empty()) {
        LOG_WARN("LaunchBox: no clear logo for " + table.bestTitle);
        return;
    }

    std::string remoteFilename = images["Clear Logo"][0].get<std::string>();
    std::string url = settings_.lbdbImgUrl + remoteFilename;  // lbdbImgUrl = "https://images.launchbox-app.com/";

    fs::path dir = settings_.topperStillImages;
    fs::create_directories(dir);

    fs::path output = dir / (gameId + ".png");

    // Skip if logo already exists
    if (fs::exists(output)) {
        LOG_INFO("Clear Logo already exists → skipping download: " + output.string());
        // table.clearLogo = output.string(); // ensure metadata preserved
        return;
    }

    if (!data::filedownloader::downloadFile(url, output)) {
        LOG_ERROR("Failed to download clear logo → " + url);
        return;
    }

    LOG_INFO("Downloaded Clear Logo → " + table.bestTitle + " → " + output.string());

    if (!lbdb::resizeClearLogo(output, 128, 32)) {
        LOG_WARN("Resize failed for Clear Logo → " + output.string());
        return;
    }

    LOG_INFO("Resized Clear Logo to 128x32 → " + output.string());
}


void LbdbScanner::downloadFlyersFromJson(const std::string& gameId,
                                            TableData& table,
                                            const nlohmann::json& db) {
    auto it = std::find_if(db.begin(), db.end(),
        [&gameId](const auto& g) {
            return g["Id"].template get<std::string>() == gameId;
        });

    if (it == db.end() || !it->contains("images")) return;

    const auto& images = (*it)["images"];

    fs::path tableDir = fs::path(table.folder);

    auto download = [&](const std::string& filename, const fs::path& localPath) {
        fs::create_directories(localPath.parent_path());

        // Skip if flyer already exists
        if (fs::exists(localPath)) {
            LOG_INFO("Flyer already exists → skipping download: " + localPath.string());
            return true; // treat as success
        }

        std::string url = settings_.lbdbImgUrl + filename;  // lbdbImgUrl = "https://images.launchbox-app.com/";
        return data::filedownloader::downloadFile(url, localPath);
    };

    if (images.contains("Advertisement Flyer - Front") &&
        !images["Advertisement Flyer - Front"].empty()) {

        std::string remote = images["Advertisement Flyer - Front"][0].get<std::string>();
        fs::path local = tableDir / settings_.customFlyerFrontImage;

        if (download(remote, local)) table.flyerFront = local.string();
    }

    if (images.contains("Advertisement Flyer - Back") &&
        !images["Advertisement Flyer - Back"].empty()) {

        std::string remote = images["Advertisement Flyer - Back"][0].get<std::string>();
        fs::path local = tableDir / settings_.customFlyerBackImage;

        if (download(remote, local)) table.flyerBack = local.string();
    }
}
