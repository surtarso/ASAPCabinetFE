#include "vpsdb_catalog_json.h"
#include <nlohmann/json.hpp>
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

namespace vpsdb {

VpsdbJsonLoader::VpsdbJsonLoader(const Settings& settings)
    : settings_(settings),
      loaded_(false),
      isLoading_(true),
      progressStage_(0),
      vpsDbClient_(std::make_unique<VpsDatabaseClient>(settings)) {
    initThread_ = std::thread(&VpsdbJsonLoader::initInBackground, this);
}

VpsdbJsonLoader::~VpsdbJsonLoader() {
    if (initThread_.joinable()) {
        initThread_.join();
    }
}

void VpsdbJsonLoader::initialize() {
    if (initThread_.joinable()) {
        initThread_.join();
    }
}

void VpsdbJsonLoader::waitForInit() {
    // We assume initThread_ is the thread started by initInBackground.
    // If it's joinable, it means it's running or just finished.
    if (initThread_.joinable()) {
        LOG_INFO("VpsdbJsonLoader: Waiting for background initialization to complete.");
        initThread_.join(); // BLOCKING CALL: Wait for the thread to finish
        LOG_INFO("VpsdbJsonLoader: Background initialization complete.");
    }
}

void VpsdbJsonLoader::initInBackground() {
    isLoading_ = true;
    progressStage_ = 1;
    LOG_DEBUG("VpsdbJsonLoader: Starting initialization in background");

    if (!fs::exists(settings_.vpsDbPath)) {
        LOG_DEBUG("VpsdbJsonLoader: vpsdb.json not found, initiating fetch");
        if (!vpsDbClient_->fetchIfNeeded(settings_.vpsDbLastUpdated, settings_.vpsDbUpdateFrequency, nullptr)) {
            LOG_ERROR("VpsdbJsonLoader: Failed to fetch vpsdb.json");
            isLoading_ = false;
            progressStage_ = 0;
            return;
        }
    } else {
        LOG_DEBUG("VpsdbJsonLoader: vpsdb.json exists, checking for updates");
        if (!vpsDbClient_->fetchIfNeeded(settings_.vpsDbLastUpdated, settings_.vpsDbUpdateFrequency, nullptr)) {
            LOG_DEBUG("VpsdbJsonLoader: vpsdb.json exists but update check failed, proceeding with current file");
        }
    }
    progressStage_ = 2;
    loadJson();
    progressStage_ = 3;
    isLoading_ = false;
    LOG_DEBUG("VpsdbJsonLoader: Initialization complete in background");
}

void VpsdbJsonLoader::loadJson() {
    // 1. Delegate loading to the client.
    // This ensures vpsDbClient_ (and its internal loader) actually holds the data.
    if (!vpsDbClient_->load(nullptr)) {
        LOG_ERROR("VpsdbJsonLoader: Failed to load via VpsDatabaseClient");
        loaded_ = false;
        return;
    }

    // 2. Retrieve the loaded data (which VpsDatabaseLoader ensures is the Tables Array)
    const nlohmann::json& tables = vpsDbClient_->getLoadedVpsDb();

    index_.clear();

    try {
        // 3. Build the index from the loaded array
        // Note: VpsDatabaseLoader::load already validates this is an array or extracts "tables"
        if (tables.is_array()) {
            for (const auto& entry : tables) {
                TableIndex idx;
                idx.id = entry.value("id", "");
                idx.name = entry.value("name", "");
                idx.manufacturer = entry.value("manufacturer", "");
                idx.year = entry.value("year", 0);
                index_.push_back(idx);
            }
            loaded_ = true;
            LOG_INFO("VpsdbJsonLoader: Loaded " + std::to_string(index_.size()) + " tables from JSON");
        } else {
            LOG_ERROR("VpsdbJsonLoader: Loaded data is not an array");
            loaded_ = false;
        }
    } catch (const std::exception& e) {
        LOG_ERROR("VpsdbJsonLoader: Index building error: " + std::string(e.what()));
        loaded_ = false;
    }
}

const nlohmann::json& VpsdbJsonLoader::getVpsDb() const {
    if (!vpsDbClient_) {
        // Return a static empty JSON object if the client hasn't been initialized
        static const nlohmann::json emptyJson = nlohmann::json::object();
        LOG_ERROR("VpsDatabaseClient is null in VpsdbJsonLoader::getVpsDb()");
        return emptyJson;
    }
    // This forwards the call to the VpsDatabaseClient, which holds the actual data.
    return vpsDbClient_->getLoadedVpsDb();
}

} // namespace vpsdb
