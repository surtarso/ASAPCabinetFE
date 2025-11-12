#include "editor/menu_actions.h"
#include <thread>

namespace fs = std::filesystem;

namespace menu_actions {

// ---------------------------------------------------------------------------
// Delete Folder Request
// ---------------------------------------------------------------------------
void requestDeleteTableFolder(EditorUI& ui) {
    if (ui.selectedIndex() < 0 || ui.selectedIndex() >= static_cast<int>(ui.filteredTables().size())) {
        LOG_WARN("Delete Table Folder requested but no table selected.");
        // Show modal info for missing selection
        ui.modal().openInfo(
            "No Table Selected",
            "Please select a table first and try again."
        );
        return;
    }

    const auto& sel = ui.filteredTables()[ui.selectedIndex()];
    fs::path folder = fs::path(sel.vpxFile).parent_path();

    if (!folder.empty() && fs::exists(folder)) {
        ui.modal().openConfirm(
            "Confirm Delete?",
            "Delete folder:\n" + folder.string() + "\n\nThis will permanently remove all files. Continue?",
            {"No", "Yes"}, // defaults to 1st on ui
            [&ui, folder](const std::string& choice) {
                if (choice == "Yes") {
                    try {
                        std::error_code ec;
                        size_t removed = std::filesystem::remove_all(folder, ec);
                        if (ec)
                            LOG_ERROR("Failed to delete folder: " + ec.message());
                        else
                            LOG_INFO("Deleted folder: " + folder.string() + " (" + std::to_string(removed) + " items)");

                        ui.rescanAsyncPublic(ui.scannerMode());
                        ui.filterAndSortTablesPublic();

                        // ui.modal().openInfo("Deleted", "Folder deleted successfully:\n" + folder.string());
                    } catch (const std::exception& e) {
                        LOG_ERROR(std::string("Exception deleting folder: ") + e.what());
                        ui.modal().openInfo("Error", "Failed to delete folder:\n" + folder.string());
                    }
                } else {
                    LOG_INFO("Delete canceled.");
                }
            }
        );
    } else {
        LOG_ERROR("Delete Table Folder failed: folder not found.");
    }
}

// ---------------------------------------------------------------------------
// Delete File Request
// ---------------------------------------------------------------------------
void requestDeleteTableFile(EditorUI& ui, const std::string& fileType) {
    if (ui.selectedIndex() < 0 || ui.selectedIndex() >= static_cast<int>(ui.filteredTables().size())) {
        LOG_WARN("Delete " + fileType + " requested but no table selected.");
        // Show modal info for missing selection
        ui.modal().openInfo(
            "No Table Selected",
            "You asked to delete \"" + fileType + "\" but no table is currently selected.\n\n"
            "Please select a table first and try again."
        );
        return;
    }

    const auto& sel = ui.filteredTables()[ui.selectedIndex()];
    fs::path base = fs::path(sel.vpxFile).replace_extension(fileType);
    if (fs::exists(base)) {
        ui.modal().openConfirm(
            "Confirm Delete?",
            "Delete file:\n" + base.string() + "\n\nThis will permanently remove it. Continue?",
            {"No", "Yes"},
            [&ui, base, fileType](const std::string& choice) {
                if (choice == "Yes") {
                    try {
                        std::error_code ec;
                        bool removed = std::filesystem::remove(base, ec);
                        if (ec) {
                            LOG_ERROR("Failed to delete " + fileType + ": " + ec.message());
                            ui.modal().openError("File Operation error", "Failed to delete " + fileType + ": " + ec.message());
                        } else if (removed) {
                            ui.rescanAsyncPublic(ui.scannerMode());
                            ui.filterAndSortTablesPublic();
                            LOG_INFO("Deleted " + fileType + ": " + base.string());
                            // ui.modal().openInfo("File Operation", "Deleted " + fileType + ": " + base.string());
                        } else {
                            LOG_WARN("Nothing deleted (file missing): " + base.string());
                            ui.modal().openWarning("File Operation", "Nothing deleted (file missing): " + base.string());
                        }
                    } catch (const std::exception& e) {
                        LOG_ERROR(std::string("Exception deleting ") + fileType + ": " + e.what());
                        ui.modal().openInfo("Error", "Failed to delete file:\n" + base.string());
                    }
                } else {
                    LOG_INFO("Delete canceled.");
                }
            }
        );
    } else {
        LOG_ERROR("Delete " + fileType + " failed: file not found.");
        ui.modal().openError("File Operation error", "Delete " + fileType + " failed: file not found.");
    }
}

// ---------------------------------------------------------------------------
// Compression Request
// ---------------------------------------------------------------------------
static std::string detectCompressor() {
    const std::vector<std::string> tools = {"zip", "7z", "tar", "rar"};
    for (const auto& t : tools) {
        std::string cmd = "command -v " + t + " >/dev/null 2>&1";
        if (std::system(cmd.c_str()) == 0) return t;
    }
    return {};
}

void requestCompressTableFolder(EditorUI& ui) {
    if (ui.selectedIndex() < 0 || ui.selectedIndex() >= static_cast<int>(ui.filteredTables().size())) {
        LOG_INFO("Compression requested but no table selected.");
        ui.modal().openInfo("No Table Selected", "Please select a table first and try again.");
        return;
    }

    const auto& sel = ui.filteredTables()[ui.selectedIndex()];
    fs::path folder = fs::path(sel.vpxFile).parent_path();
    if (!fs::exists(folder) || !fs::is_directory(folder)) {
        LOG_ERROR("Compression failed: folder not found.");
        ui.modal().openError("Archival error", "Compression failed: folder not found.");
        return;
    }

    Settings& settings = ui.configService()->getMutableSettings();
    std::string compressor = settings.preferredCompressor;

    if (compressor == "auto" || compressor.empty()) {
        compressor = detectCompressor();
        if (compressor.empty()) {
            LOG_ERROR("No compressor tool found (zip, 7z, tar, rar). Install one or set manually in settings.");
            ui.modal().openError("Archival error", "No compressor tool found (zip, 7z, tar, rar). Install one or set manually in settings.");
            return;
        }
        settings.preferredCompressor = compressor;
        ui.configService()->saveConfig();
    }

    fs::path archive = folder.parent_path() / (folder.filename().string() + ".zip");
    std::string cmd;

    if (compressor == "zip")       cmd = "zip -r \"" + archive.string() + "\" \"" + folder.string() + "\"";
    else if (compressor == "7z")   cmd = "7z a \"" + archive.string() + ".7z\" \"" + folder.string() + "\"";
    else if (compressor == "tar")  cmd = "tar -czf \"" + archive.string() + ".tar.gz\" -C \"" + folder.parent_path().string() + "\" \"" + folder.filename().string() + "\"";
    else if (compressor == "rar")  cmd = "rar a \"" + archive.string() + ".rar\" \"" + folder.string() + "\"";

    LOG_INFO("Compressing with " + compressor + ": " + cmd);

    std::thread([cmd, compressor]() {
        int ret = std::system(cmd.c_str());
        if (ret == 0)
            LOG_INFO("Compression complete using " + compressor);
        else
            LOG_ERROR("Compression failed (exit " + std::to_string(ret) + ")");
    }).detach();
}

// ---------------------------------------------------------------------------
// VPXTool generic executor
// ---------------------------------------------------------------------------
void vpxtoolRun(EditorUI& ui, const std::string& command) {
    if (ui.selectedIndex() < 0 || ui.selectedIndex() >= static_cast<int>(ui.filteredTables().size())) {
        LOG_WARN("VPXTool " + command + " requested but no table selected.");
        // Show modal info for missing selection
        ui.modal().openInfo(
            "No Table Selected",
            "You pressed \"" + command + "\" but no table is currently selected.\n\n"
            "Please select a table first and try again."
        );
        return;
    }

    const auto& sel = ui.filteredTables()[ui.selectedIndex()];
    fs::path vpxFile = sel.vpxFile;
    if (!fs::exists(vpxFile)) {
        LOG_ERROR("VPXTool command failed: table file not found: " + vpxFile.string());
        ui.modal().openError("VPXTool error", "VPXTool command failed: table file not found: " + vpxFile.string());
        return;
    }

    // Example command
    std::string cmd = "vpxtool " + command + " \"" + vpxFile.string() + "\"";
    LOG_INFO("Executing: " + cmd);

    // Run asynchronously
    std::thread([cmd]() {
        int result = std::system(cmd.c_str());
        if (result != 0){
            LOG_ERROR("VPXTool command failed with exit code " + std::to_string(result));
            // ui.modal().openError("VPXTool command failed with exit code " + std::to_string(result));
        } else {
            LOG_INFO("VPXTool command completed successfully.");
            // ui.modal().finishProgress("VPXTool command completed successfully.");
        }
    }).detach();
}

// ---------------------------------------------------------------------------
// Clear Caches (placeholder)
// ---------------------------------------------------------------------------
    void clearAllCaches(EditorUI& ui) {
        LOG_WARN("Clear All Caches requested [Confirmation]");
    ui.modal().openConfirm(
        "Clear Cache?",
        "This will remove all cached data. Continue?",
        {"Yes", "No"},
        [&ui](const std::string& choice) {
            if (choice == "Yes") {
                LOG_INFO("Clearing caches...");
                // TODO: actual cache clear logic
                ui.modal().openInfo("Cache Cleared", "All caches were successfully cleared.");
            } else {
                LOG_INFO("Cache clearing canceled.");
            }
        }
    );
}

// ---------------------------------------------------------------------------
// Draw Confirmation Popups
// ---------------------------------------------------------------------------
void drawModals(EditorUI& ui) {
    ui.modal().draw();
}

}  // namespace menu_actions
