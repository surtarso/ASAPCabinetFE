// src/data/asapcab/asapcab_database_manager.cpp
#include "asapcab_database_manager.h"
#include "asapcab_database_builder.h"
#include "log/logging.h"

// Updater / Loader headers (use the concrete APIs present in the repo)
#include "data/vpsdb/vps_database_updater.h"
#include "data/vpsdb/vps_database_loader.h"

#include "data/lbdb/lbdb_updater.h"
#include "data/lbdb/lbdb_loader.h"

#include "data/vpinmdb/vpinmdb_updater.h"
#include "data/vpinmdb/vpinmdb_loader.h"

#include "data/ipdb/ipdb_updater.h"
#include "data/ipdb/ipdb_loader.h"

#include <fstream>

namespace fs = std::filesystem;
using json = nlohmann::json;

namespace data::asapcabdb {

AsapCabDatabaseManager::AsapCabDatabaseManager(const Settings& settings)
    : settings_(settings)
{
    // keep using the Settings-backed path; it's trivial to change later
    asapcabDbPath_ = settings_.mainDbPath;
}

bool AsapCabDatabaseManager::ensureAvailable() {
    if (fs::exists(asapcabDbPath_) && isUpToDate()) {
        LOG_INFO("Master ASAPCab DB exists and appears up to date");
        return true;
    }

    LOG_WARN("Master ASAPCab DB missing or stale → rebuilding");
    return build();
}

bool AsapCabDatabaseManager::isUpToDate() const {
    // MVP: always false → always rebuild
    // Later: check timestamps or metadata checksums
    return false;
}

json AsapCabDatabaseManager::load() {
    if (!fs::exists(asapcabDbPath_)) {
        LOG_ERROR("Master DB missing: " + asapcabDbPath_.string());
        return {};
    }

    std::ifstream f(asapcabDbPath_);
    if (!f.is_open()) {
        LOG_ERROR("Cannot open master DB: " + asapcabDbPath_.string());
        return {};
    }

    try {
        json j;
        f >> j;
        return j;
    } catch (...) {
        LOG_ERROR("Master DB JSON invalid: " + asapcabDbPath_.string());
        return {};
    }
}

bool AsapCabDatabaseManager::build() {
    LOG_INFO("Building Master Database (ASAPCab)…");

    // -------------------------
    // 1) Ensure source DBs present
    // -------------------------

    // VPS (these classes take file-path-based ctor / free namespace)
    {
        // VpsDatabaseUpdater expects the vpsdb path string (see vps_database_updater.h/.cpp).
        VpsDatabaseUpdater vpsUpdater(settings_.vpsDbPath);
        // fetchIfNeeded expects (lastUpdatedPath, updateFrequency, LoadingProgress*)
        if (!vpsUpdater.fetchIfNeeded(settings_.vpsDbLastUpdated, settings_.vpsDbUpdateFrequency, nullptr)) {
            LOG_ERROR("VPS DB unavailable");
            return false;
        }
    }

    // LaunchBox (lbdb) - namespaced updater that takes Settings + progress
    {
        data::lbdb::LbdbUpdater lbUpdater(settings_, nullptr);
        if (!lbUpdater.ensureAvailable()) {
            LOG_ERROR("LaunchBox DB unavailable");
            return false;
        }
    }

    // VPinMDB (media DB) - namespaced updater
    {
        data::vpinmdb::VpinMdbUpdater vpinUpdater(settings_, nullptr);
        if (!vpinUpdater.ensureAvailable()) {
            LOG_ERROR("VPinMDB unavailable");
            return false;
        }
    }

    // IPDB - namespaced updater (data::ipdb::IpdbUpdater)
    {
        data::ipdb::IpdbUpdater ipdbUpdater(settings_, nullptr);
        if (!ipdbUpdater.ensureAvailable()) {
            LOG_ERROR("IPDB unavailable");
            return false;
        }
    }

    // -------------------------
    // 2) Load each DB into JSON
    // -------------------------
    json db_vpsdb;
    json db_lbdb;
    json db_vpinmdb;
    json db_ipdb;

    // VPS loader (path-based)
    {
        VpsDatabaseLoader loader(settings_.vpsDbPath);
        if (!loader.load(nullptr)) {
            LOG_ERROR("Failed to load VPSDB");
            return false;
        }
        db_vpsdb = loader.getVpsDb();
    }

    // LaunchBox loader
    {
        data::lbdb::LbdbLoader loader(settings_, nullptr);
        db_lbdb = loader.load();
        if (db_lbdb.empty()) {
            LOG_ERROR("Failed to load LaunchBox DB");
            return false;
        }
    }

    // VPinMDB loader
    {
        data::vpinmdb::VpinMdbLoader loader(settings_, nullptr);
        try {
            db_vpinmdb = loader.load();
        } catch (const std::exception& e) {
            LOG_ERROR(std::string("Failed to load VPinMDB: ") + e.what());
            return false;
        }
        if (db_vpinmdb.empty()) {
            LOG_ERROR("VPinMDB loaded but empty");
            return false;
        }
    }

    // IPDB loader
    {
        data::ipdb::IpdbLoader loader(settings_, nullptr);
        db_ipdb = loader.load();
        if (db_ipdb.empty()) {
            LOG_ERROR("Failed to load IPDB");
            return false;
        }
    }

    // -------------------------
    // 3) Assemble master JSON
    // -------------------------
    AsapCabDatabaseBuilder builder;
    json masterDb = builder.build(db_vpsdb, db_lbdb, db_vpinmdb, db_ipdb);
    if (!writeMasterJson(masterDb)) {
        LOG_ERROR("Failed writing master database");
        return false;
    }

    // -------------------------
    // 4) Save master file
    // -------------------------
    if (!writeMasterJson(masterDb)) {
        LOG_ERROR("Failed writing master database");
        return false;
    }

    LOG_INFO("ASAPCab master database built successfully");
    return true;
}

bool AsapCabDatabaseManager::writeMasterJson(const json& j) {
    try {
        fs::create_directories(asapcabDbPath_.parent_path());
        std::ofstream f(asapcabDbPath_);
        if (!f.is_open()) return false;
        f << j.dump(2);
        return true;
    } catch (...) {
        return false;
    }
}

} // namespace data::asapcabdb
