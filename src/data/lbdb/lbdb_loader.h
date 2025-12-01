#pragma once
/**
 * lbdb_loader.h
 *
 * Loads launchbox_pinball.json from disk and returns nlohmann::json.
 *
 */

#include <nlohmann/json.hpp>
#include "config/settings.h"
#include "core/ui/loading_progress.h"
#include <filesystem>

namespace data::lbdb {

namespace fs = std::filesystem;
using json = nlohmann::json;

class LbdbLoader {
public:
    LbdbLoader(const Settings& settings, LoadingProgress* progress)
        : settings_(settings), progress_(progress) {}

    // Loads and returns the full DB.
    // Returns empty json on failure (matching previous behavior).
    json load();

private:
    const Settings& settings_;
    LoadingProgress* progress_;

    void push(const std::string& msg);
};

} // namespace data::lbdb
