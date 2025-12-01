#pragma once
/**
 * vpinmdb_loader.h
 *
 * Small loader that reads local vpinmdb.json and returns nlohmann::json.
 * Throws std::runtime_error on fatal I/O or parsing errors.
 *
 */

#include <nlohmann/json.hpp>
#include <string>
#include <filesystem>
#include <optional>
#include "config/settings.h"
#include "core/ui/loading_progress.h"

namespace data::vpinmdb {

namespace fs = std::filesystem;
using json = nlohmann::json;

class VpinMdbLoader {
public:
    VpinMdbLoader(const Settings& settings, LoadingProgress* progress)
        : settings_(settings), progress_(progress) {}

    // Load and parse the local vpinmdb.json. Throws std::runtime_error on failure.
    json load();

private:
    const Settings& settings_;
    LoadingProgress* progress_;

    void pushProgressMessage(const std::string &msg);
};

} // namespace data::vpinmdb
