#ifndef DATA_IPDB_IPDB_DOWNLOADER_H
#define DATA_IPDB_IPDB_DOWNLOADER_H

#include <memory>
#include "config/settings.h"
#include "core/ui/loading_progress.h"

namespace data::ipdb {

class IpdbDownloader {
public:
    IpdbDownloader(Settings& settings,
                   std::shared_ptr<LoadingProgress> progress = nullptr);

    // (future) scan process will use this returned JSON
    void updateIfNecessary();

private:
    Settings& settings_;
    std::shared_ptr<LoadingProgress> progress_;
};

} // namespace data::ipdb

#endif
