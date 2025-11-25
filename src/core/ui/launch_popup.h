// launch_popup.h
#pragma once
#include <string>

struct LaunchPopup {
    bool active = false;
    bool failed = false;
    std::string tableName;
};
