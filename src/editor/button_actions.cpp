#include "editor/button_actions.h"
#include "vpin_wrapper.h"
#include "log/logging.h"
#include <filesystem>
#include <cstdlib>

namespace fs = std::filesystem;

ButtonActions::ButtonActions(IConfigService* config)
    : config_(config) {}

void ButtonActions::extractVBS(const std::string& filepath) {
    if (!config_) {
        LOG_ERROR("Config service is null, cannot extract VBS.");
        return;
    }
    const auto& settings = config_->getSettings();

    // Check if the user wants to use the external vpxtool
    if (settings.useVpxtool) {
        // --- External vpxtool Logic ---
        LOG_DEBUG("Using external 'vpxtool' for VBS extraction.");

        // Use vpxtoolBin setting, fallback to PATH
        std::string vpxtoolPath = "vpxtool";
        if (!settings.vpxtoolBin.empty() && fs::exists(settings.vpxtoolBin)) {
            vpxtoolPath = settings.vpxtoolBin;
        } else if (!settings.vpxtoolBin.empty()) {
            LOG_WARN("vpxtoolBin setting is specified but not found: " + settings.vpxtoolBin + ". Falling back to PATH.");
        }

        std::string cmd = "\"" + vpxtoolPath + "\" " + settings.vpxtoolExtractCmd + " \"" + filepath + "\"";
        LOG_DEBUG("Extracting VBS with command: " + cmd);
        int result = system(cmd.c_str());
        if (result != 0) {
            LOG_ERROR("Failed to extract VBS from table: " + filepath + " (command: " + cmd + ")");
        }
    } else {
        // --- Internal vpin_wrapper Logic (Placeholder) ---
        LOG_WARN("Placeholder: Internal VBS extraction is not yet implemented in vpin_wrapper.");
        // Example:
        //
        // bool success = extract_vbs_from_vpx(filepath.c_str());
        // if (!success) {
        //     LOG_ERROR("Internal VBS extraction failed for: " + filepath);
        // }
    }
}

bool ButtonActions::openInExternalEditor(const std::string& filepath) {
    std::string cmd = "xdg-open \"" + filepath + "\"";
    LOG_DEBUG("Attempting to open in external editor with: " + cmd);
    if (system(cmd.c_str()) != 0) {
        LOG_WARN("xdg-open failed. (You could add a 'fallbackEditor' to your Settings struct like in the old app).");
        return false;
    }
    return true;
}

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
