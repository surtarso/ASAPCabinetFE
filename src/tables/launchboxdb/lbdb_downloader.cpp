// src/tables/launchboxdb/lbdb_downloader.cpp
#include "lbdb_downloader.h"
#include "lbdb_builder.h"
#include "log/logging.h"
#include "tables/vpinmdb/vpinmdb_downloader.h"
#include "tables/vpinmdb/vpinmdb_image.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <sstream>

namespace fs = std::filesystem;

void LbdbDownloader::downloadArtForTables(std::vector<TableData>& tables) {
    fs::path jsonPath = settings_.lbdbPath; // "data/cache/launchbox_pinball.json";

    // First-run behavior → auto-build DB
    if (!fs::exists(jsonPath)) {
        LOG_WARN("LaunchBox DB missing — building automatically...");

        // Blocking: user sees progress UI, avoids silent confusion
        bool success = launchbox::build_pinball_database(settings_);
        if (!success) {
            LOG_ERROR("LaunchBox DB auto-build failed");
            return;
        }
        LOG_INFO("LaunchBox DB auto-build succeeded");
    }

    static nlohmann::json pinballDb;
    if (pinballDb.empty()) {
        LOG_INFO("Loading launchbox_pinball.json...");
        std::ifstream f(jsonPath);
        if (!f.is_open()) {
            LOG_ERROR("Failed to open launchbox_pinball.json");
            return;
        }
        try { f >> pinballDb; }
        catch (...) {
            LOG_ERROR("Invalid JSON in launchbox_pinball.json");
            return;
        }
        LOG_INFO("Loaded " + std::to_string(pinballDb.size()) + " pinball games from LaunchBox DB");
    }

    size_t processed = 0;
    for (auto& table : tables) {
        std::string key = table.title;
        if (!table.year.empty()) key += " " + table.year;
        if (!table.manufacturer.empty()) key += " " + table.manufacturer;
        std::transform(key.begin(), key.end(), key.begin(), ::tolower);

        std::string bestId;
        int bestScore = 0;

        for (const auto& game : pinballDb) {
            std::string gtitle = game["Name"].get<std::string>();
            std::string gyear  = game.value("Year", "");
            std::string gdev   = game.value("Developer", "");

            std::string gkey = gtitle;
            if (!gyear.empty()) gkey += " " + gyear;
            if (!gdev.empty()) gkey += " " + gdev;
            std::transform(gkey.begin(), gkey.end(), gkey.begin(), ::tolower);

            int score = 0;
            if (gkey.find(key) != std::string::npos || key.find(gkey) != std::string::npos) score += 100;
            if (!table.year.empty() && gyear == table.year) score += 80;
            if (!table.manufacturer.empty() && !gdev.empty() &&
                (gdev.find(table.manufacturer) != std::string::npos || table.manufacturer.find(gdev) != std::string::npos))
                score += 60;

            if (score > bestScore) {
                bestScore = score;
                bestId = game["Id"].get<std::string>();
            }
        }

        if (bestScore >= 100 && !bestId.empty()) {
            LOG_INFO("LaunchBox MATCH → " + table.title + " (score: " + std::to_string(bestScore) + ")");

            table.lbdbID = bestId;

            downloadClearLogo(bestId, table, pinballDb);
            downloadFlyersFromJson(bestId, table, pinballDb);
        }

        processed++;
        if (progress_) {
            std::lock_guard<std::mutex> l(progress_->mutex);
            progress_->currentTablesLoaded = processed;
            progress_->logMessages.push_back("LaunchBox: " + table.title);
        }
    }
}

// Classic DMD logo (always just gameId.png)
void LbdbDownloader::downloadClearLogo(const std::string& gameId,
                                       TableData& table,
                                       const nlohmann::json& db) {
    auto it = std::find_if(db.begin(), db.end(),
        [&gameId](const auto& g) {
            return g["Id"].template get<std::string>() == gameId;
        });

    if (it == db.end() || !it->contains("images")) {
        LOG_WARN("LaunchBox: no image block for " + table.title);
        return;
    }

    const auto& images = (*it)["images"];

    if (!images.contains("Clear Logo") || images["Clear Logo"].empty()) {
        LOG_WARN("LaunchBox: no clear logo for " + table.title);
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

    if (!vpinmdb::downloadFile(url, output)) {
        LOG_ERROR("Failed to download clear logo → " + url);
        return;
    }

    LOG_INFO("Downloaded Clear Logo → " + table.title + " → " + output.string());

    if (!vpinmdb::resizeImage(output, 128, 32)) {
        LOG_WARN("Resize failed for Clear Logo → " + output.string());
        return;
    }

    LOG_INFO("Resized Clear Logo to 128x32 → " + output.string());

    // store path in metadata
    // table.clearLogo = output.string();
}


void LbdbDownloader::downloadFlyersFromJson(const std::string& gameId,
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
        return vpinmdb::downloadFile(url, localPath);
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
