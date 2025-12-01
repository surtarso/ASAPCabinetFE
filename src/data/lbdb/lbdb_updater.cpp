#include "lbdb_updater.h"
#include "data/lbdb/lbdb_builder.h"
#include "log/logging.h"

namespace data::lbdb {

void LbdbUpdater::push(const std::string& msg) {
    if (!progress_) return;
    std::lock_guard<std::mutex> lock(progress_->mutex);
    progress_->logMessages.push_back(msg);
}

bool LbdbUpdater::ensureAvailable() {
    fs::path jsonPath = settings_.lbdbPath;

    if (fs::exists(jsonPath)) {
        LOG_INFO("LaunchBox DB already present at " + jsonPath.string());
        return true;
    }

    LOG_WARN("LaunchBox DB missing — building automatically...");
    push("LaunchBox DB not found — building...");

    bool ok = launchbox::build_pinball_database(settings_);
    if (!ok) {
        LOG_ERROR("LaunchBox DB auto-build failed");
        push("LaunchBox DB build failed");
        return false;
    }

    LOG_INFO("LaunchBox DB auto-build succeeded");
    push("LaunchBox DB build succeeded");
    return true;
}

} // namespace data::lbdb
