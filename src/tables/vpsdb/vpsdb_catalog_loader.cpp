#include "vpsdb_catalog_loader.h"
#include <nlohmann/json.hpp>
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

namespace vpsdb {

VpsdbJsonLoader::VpsdbJsonLoader(const std::string& vpsdbFilePath, const Settings& settings)
    : vpsdbFilePath_(vpsdbFilePath),
      settings_(settings),
      loaded_(false),
      isLoading_(true),
      progressStage_(0),
      vpsDbClient_(std::make_unique<VpsDatabaseClient>(vpsdbFilePath)) {
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

void VpsdbJsonLoader::initInBackground() {
    isLoading_ = true;
    progressStage_ = 1;
    LOG_DEBUG("VpsdbJsonLoader: Starting initialization in background");

    if (!fs::exists(vpsdbFilePath_)) {
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
    try {
        std::ifstream file(vpsdbFilePath_);
        if (!file.is_open()) {
            LOG_ERROR("VpsdbJsonLoader: Failed to open JSON file: " << vpsdbFilePath_);
            loaded_ = false;
            return;
        }
        nlohmann::json json;
        file >> json;
        index_.clear();
        for (const auto& entry : json) {
            TableIndex idx;
            idx.id = entry.value("id", "");
            idx.name = entry.value("name", "");
            idx.manufacturer = entry.value("manufacturer", "");
            idx.year = entry.value("year", 0);
            index_.push_back(idx);
        }
        loaded_ = true;
        LOG_INFO("VpsdbJsonLoader: Loaded " << index_.size() << " tables from JSON");
    } catch (const std::exception& e) {
        LOG_ERROR("VpsdbJsonLoader: JSON parsing error: " << e.what());
        loaded_ = false;
    }
}

} // namespace vpsdb