#pragma once
/**
 * lbdb_updater.h
 *
 * Ensures the LaunchBox pinball DB exists.
 * Wraps launchbox::build_pinball_database(settings_).
 * Does NOT load JSON or match tables — only prepares the file on disk.
 */

#include <filesystem>
#include "config/settings.h"
#include "core/ui/loading_progress.h"

namespace data::lbdb {

namespace fs = std::filesystem;

class LbdbUpdater {
public:
    LbdbUpdater(const Settings& settings, LoadingProgress* progress)
        : settings_(settings), progress_(progress) {}

    // Ensures launchbox_pinball.json exists (builds it if missing).
    // Returns true if DB exists after this call.
    // Returns false on failure.
    bool ensureAvailable();

private:
    const Settings& settings_;
    LoadingProgress* progress_;

    void push(const std::string& msg);
};

} // namespace data::lbdb
