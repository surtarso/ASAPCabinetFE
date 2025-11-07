#include "editor/button_actions.h"
#include "log/logging.h"
#include <filesystem>
#include <cstdlib>

namespace fs = std::filesystem;

ButtonActions::ButtonActions(IConfigService* config)
    : config_(config) {}

void ButtonActions::openFolder(const std::string& filepath) {
    std::string folder;
    if (filepath.empty()) {
        folder = config_ ? config_->getSettings().VPXTablesPath : "."; // fallback to tables dir if possible
    } else {
        fs::path filePathObj(filepath);
        folder = filePathObj.parent_path().string();
    }

    if (folder.empty() || !fs::exists(folder)) {
        LOG_ERROR("Cannot open folder, invalid path: " + folder);
        return;
    }

    std::string cmd = "xdg-open \"" + folder + "\"";
    int result = system(cmd.c_str());
    if (result != 0) {
        LOG_ERROR("Failed to open folder: " + folder + " (exit code " + std::to_string(result) + ")");
    } else {
        LOG_DEBUG("Opened folder: " + folder);
    }
}
