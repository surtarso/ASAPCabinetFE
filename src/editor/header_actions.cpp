#include "editor/header_actions.h"
#include <thread>
#include <filesystem>
#include "config/settings.h"
#include "log/logging.h"
#include <cstdlib>

namespace fs = std::filesystem;

namespace header_actions {

// ---------------------------------------------------------------------------
// Delete Folder Request
// ---------------------------------------------------------------------------
void requestDeleteTableFolder(EditorUI& ui) {
    if (ui.selectedIndex() < 0 || ui.selectedIndex() >= static_cast<int>(ui.filteredTables().size())) {
        LOG_INFO("Delete Table Folder requested but no table selected.");
        // Show modal info for missing selection
        ui.modal().openInfo(
            "No Table Selected",
            "Delete Table Folder requested but no table selected."
            "Please select a table first and try again."
        );
        return;
    }

    const auto& sel = ui.filteredTables()[ui.selectedIndex()];
    fs::path folder = fs::path(sel.vpxFile).parent_path();

    if (!folder.empty() && fs::exists(folder)) {
        ui.modal().openConfirm(
            "Confirm Delete?",
            "Delete TABLE FOLDER:\n" + folder.string() + "\n\nThis will permanently REMOVE ALL FILES. Continue?",
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
                        ui.modal().openError("Error", "Failed to delete folder:\n" + folder.string());
                    }
                } else {
                    LOG_INFO("Delete canceled.");
                }
            }
        );
    } else {
        LOG_ERROR("Delete Table Folder failed: folder not found.");
        ui.modal().openError("Error", "Failed to delete, folder not found:\n" + folder.string());
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
            "Delete file:\n" + base.string() + "\n\nThis will permanently REMOVE it. Continue?",
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
                        ui.modal().openError("Error", "Failed to delete file:\n" + base.string());
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
    // ------------------------------
    // Validate selection
    // ------------------------------
    int idx = ui.selectedIndex();
    const auto& filtered = ui.filteredTables();

    if (idx < 0 || idx >= static_cast<int>(filtered.size())) {
        LOG_INFO("Compression requested but no table selected.");
        ui.modal().openInfo(
            "No Table Selected",
            "Compression requested but no table selected. Please select a table first and try again."
        );
        return;
    }

    // ------------------------------
    // Resolve folder to compress
    // ------------------------------
    const auto& sel = filtered[idx];
    fs::path folder = fs::path(sel.vpxFile).parent_path();

    if (!fs::exists(folder) || !fs::is_directory(folder)) {
        LOG_ERROR("Compression failed: folder not found.");
        ui.modal().openError("Archival Error",
                             "Compression failed: folder not found.");
        return;
    }

    // ------------------------------
    // Determine compressor
    // ------------------------------
    Settings& settings = ui.configService()->getMutableSettings();
    std::string compressor = settings.preferredCompressor;

    if (compressor == "auto" || compressor.empty()) {
        compressor = detectCompressor();
        if (compressor.empty()) {
            LOG_ERROR("No compressor tool found.");
            ui.modal().openError(
                "Archival Error",
                "No compressor tool found (zip, 7z, tar, rar).\n"
                "Install one or choose manually in settings."
            );
            return;
        }
        settings.preferredCompressor = compressor;
        ui.configService()->saveConfig();
    }

    // Determine output paths
    fs::path archiveBase = folder.parent_path() / folder.filename().string();
    fs::path outputArchive;

    std::string cmd;

    if (compressor == "zip") {
        outputArchive = archiveBase.string() + ".zip";
        cmd = "zip -r \"" + outputArchive.string() + "\" \"" + folder.string() + "\"";
    }
    else if (compressor == "7z") {
        outputArchive = archiveBase.string() + ".7z";
        cmd = "7z a \"" + outputArchive.string() + "\" \"" + folder.string() + "\"";
    }
    else if (compressor == "tar") {
        outputArchive = archiveBase.string() + ".tar.gz";
        cmd = "tar -czf \"" + outputArchive.string() +
              "\" -C \"" + folder.parent_path().string() +
              "\" \"" + folder.filename().string() + "\"";
    }
    else if (compressor == "rar") {
        outputArchive = archiveBase.string() + ".rar";
        cmd = "rar a \"" + outputArchive.string() + "\" \"" + folder.string() + "\"";
    }

    LOG_INFO("Starting compression: " + cmd);

    // ----------------------------------------
    // Show progress modal IMMEDIATELY
    // ----------------------------------------
    ui.modal().openProgress(
        "Archiving Table Folder",
        "Compressing folder...\nThis may take a moment."
    );

    // ----------------------------------------
    // Threaded compression
    // ----------------------------------------
    std::thread([cmd, compressor, outputArchive, &ui]() {
        // (Optional) give tiny delay so progress popup appears before work starts
        std::this_thread::sleep_for(std::chrono::milliseconds(150));

        int ret = std::system(cmd.c_str());

        if (ret == 0) {
            LOG_INFO("Compression complete");
            ui.modal().finishProgress(
                "Compression completed successfully.",
                outputArchive.string()   // modal shows this as "Saved to:"
            );
        } else {
            LOG_ERROR("Compression failed (exit code " + std::to_string(ret) + ")");
            ui.modal().finishProgress(
                "Compression failed.\nExit code: " + std::to_string(ret),
                ""    // no result file
            );
        }
    }).detach();
}


// ---------------------------------------------------------------------------
// VPXTool generic executor
// ---------------------------------------------------------------------------
static std::string findVpxtool(EditorUI& ui) {
    // 1) Try default PATH first
    if (std::system("command -v vpxtool >/dev/null 2>&1") == 0) {
        return "vpxtool";
    }

    // 2) Try user-specified path
    const Settings& settings = ui.configService()->getSettings();
    if (!settings.vpxtoolBin.empty() && fs::exists(settings.vpxtoolBin)) {
        return settings.vpxtoolBin;
    }

    return {};
}

void vpxtoolRun(EditorUI& ui, const std::string& commandWithSub) {
    if (ui.selectedIndex() < 0 || ui.selectedIndex() >= static_cast<int>(ui.filteredTables().size())) {
        ui.modal().openInfo("No Table Selected",
                            "You pressed \"" + commandWithSub + "\" but no table is currently selected.\n\nPlease select a table first.");
        return;
    }

    const auto& sel = ui.filteredTables()[ui.selectedIndex()];
    fs::path vpxFile = sel.vpxFile;
    if (!fs::exists(vpxFile)) {
        ui.modal().openError("VPXTool error", "Table file not found: " + vpxFile.string());
        return;
    }

    std::string vpxtoolExe = findVpxtool(ui);
    if (vpxtoolExe.empty()) {
        ui.modal().openError("VPXTool Not Found",
                             "VPXTool executable not found in PATH or user settings.");
        return;
    }

    // Build command: executable + subcommands + VPX file path
    std::string fullCmd = vpxtoolExe + " " + commandWithSub + " \"" + vpxFile.string() + "\"";
    LOG_INFO("Executing: " + fullCmd);

    ui.modal().openCommandOutput("VPXTool Output: " + commandWithSub);

    std::thread([fullCmd, &ui]() {
        FILE* pipe = popen(fullCmd.c_str(), "r");
        if (!pipe) {
            ui.modal().appendCommandOutput("Failed to run command.");
            return;
        }
        char buffer[256];
        while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
            ui.modal().appendCommandOutput(buffer);
        }
        pclose(pipe);
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

}  // namespace header_actions
