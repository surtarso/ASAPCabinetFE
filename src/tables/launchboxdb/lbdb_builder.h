// src/tables/launchboxdb/lbdb_builder.h
#pragma once
#include <functional>
#include "config/settings.h"

namespace launchbox {
    bool build_pinball_database(const Settings& settings,
                    std::function<void(int current, int total)> progress = nullptr);
}
