#ifndef DATA_IPDB_IPDB_LOADER_H
#define DATA_IPDB_IPDB_LOADER_H

#include <nlohmann/json.hpp>
#include <memory>
#include "config/settings.h"
#include "core/ui/loading_progress.h"

namespace data::ipdb {

class IpdbLoader {
public:
    IpdbLoader(const Settings& settings,
            LoadingProgress* progress = nullptr);

    nlohmann::json load();

private:
    const Settings& settings_;
    LoadingProgress* progress_;
};

} // namespace data::ipdb

#endif
