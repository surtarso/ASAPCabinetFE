// src/data/asapcab/asapcab_database_manager.h
#pragma once

#include <filesystem>
#include <nlohmann/json.hpp>
#include "config/settings.h"
#include "core/ui/loading_progress.h"

// Updater/Loader headers for all DBs
#include "data/vpsdb/vps_database_updater.h"
#include "data/vpsdb/vps_database_loader.h"

#include "data/lbdb/lbdb_updater.h"
#include "data/lbdb/lbdb_loader.h"

#include "data/vpinmdb/vpinmdb_updater.h"
#include "data/vpinmdb/vpinmdb_loader.h"

#include "data/ipdb/ipdb_updater.h"
#include "data/ipdb/ipdb_loader.h"

namespace data::asapcabdb {

class AsapCabDatabaseManager {
public:
    AsapCabDatabaseManager(const Settings& settings);

    // Ensures asapcab_database.json is present.
    bool ensureAvailable();

    // Loads asapcab_database.json into a json object.
    nlohmann::json load();

    // Force a full rebuild from all source DBs.
    bool build();

private:
    bool isUpToDate() const;
    bool writeMasterJson(const nlohmann::json& j);

private:
    const Settings settings_;
    std::filesystem::path asapcabDbPath_;
};

} // namespace data::asapcabdb
