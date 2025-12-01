#include "ipdb_downloader.h"
#include "ipdb_updater.h"
#include "ipdb_loader.h"
#include "log/logging.h"

namespace data::ipdb {

IpdbDownloader::IpdbDownloader(Settings& settings,
                               std::shared_ptr<LoadingProgress> progress)
    : settings_(settings), progress_(std::move(progress)) {}

// This mirrors vpinmdb + lbdb behaviour:
// updater ensures → loader loads.
void IpdbDownloader::updateIfNecessary() {
    IpdbUpdater updater(settings_, progress_.get());
    if (!updater.ensureAvailable()) {
        LOG_ERROR("IPDB unavailable");
        return;
    }

    IpdbLoader loader(settings_, progress_.get());
    auto db = loader.load();

    if (db.empty()) {
        LOG_ERROR("IPDB loaded but empty");
        return;
    }

    LOG_INFO("IPDB available and ready.");
}

} // namespace data::ipdb
