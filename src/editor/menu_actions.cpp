#include "editor/menu_actions.h"
#include <thread>

namespace fs = std::filesystem;

namespace menu_actions {

// ---------------------------------------------------------------------------
// Internal state
// ---------------------------------------------------------------------------
static bool confirmDeletePopup = false;
static std::string pendingDeletePath;
static std::string pendingDeleteType;
static bool openDeletePopupNextFrame = false;

// ---------------------------------------------------------------------------
// Delete Folder Request
// ---------------------------------------------------------------------------
void requestDeleteTableFolder(EditorUI& ui) {
    if (ui.selectedIndex() < 0 || ui.selectedIndex() >= static_cast<int>(ui.filteredTables().size())) {
        LOG_WARN("Delete Table Folder requested but no table selected.");
        return;
    }

    const auto& sel = ui.filteredTables()[ui.selectedIndex()];
    fs::path folder = fs::path(sel.vpxFile).parent_path();

    if (!folder.empty() && fs::exists(folder)) {
        pendingDeletePath = folder.string();
        pendingDeleteType = "folder";
        confirmDeletePopup = true;
        openDeletePopupNextFrame = true;
        LOG_WARN("Delete Table Folder pending confirmation: " + pendingDeletePath);
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
        return;
    }

    const auto& sel = ui.filteredTables()[ui.selectedIndex()];
    fs::path base = fs::path(sel.vpxFile).replace_extension(fileType);
    if (fs::exists(base)) {
        pendingDeletePath = base.string();
        pendingDeleteType = fileType;
        confirmDeletePopup = true;
        openDeletePopupNextFrame = true;
        LOG_WARN("Delete " + fileType + " pending confirmation: " + pendingDeletePath);
    } else {
        LOG_ERROR("Delete " + fileType + " failed: file not found.");
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
        LOG_WARN("Compression requested but no table selected.");
        return;
    }

    const auto& sel = ui.filteredTables()[ui.selectedIndex()];
    fs::path folder = fs::path(sel.vpxFile).parent_path();
    if (!fs::exists(folder) || !fs::is_directory(folder)) {
        LOG_ERROR("Compression failed: folder not found.");
        return;
    }

    Settings& settings = ui.configService()->getMutableSettings();
    std::string compressor = settings.preferredCompressor;

    if (compressor == "auto" || compressor.empty()) {
        compressor = detectCompressor();
        if (compressor.empty()) {
            LOG_ERROR("No compressor tool found (zip, 7z, tar, rar). Install one or set manually in settings.");
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
        return;
    }

    const auto& sel = ui.filteredTables()[ui.selectedIndex()];
    fs::path vpxFile = sel.vpxFile;
    if (!fs::exists(vpxFile)) {
        LOG_ERROR("VPXTool command failed: table file not found: " + vpxFile.string());
        return;
    }

    // Example command
    std::string cmd = "vpxtool " + command + " \"" + vpxFile.string() + "\"";
    LOG_INFO("Executing: " + cmd);

    // Run asynchronously
    std::thread([cmd]() {
        int result = std::system(cmd.c_str());
        if (result != 0)
            LOG_ERROR("VPXTool command failed with exit code " + std::to_string(result));
        else
            LOG_INFO("VPXTool command completed successfully.");
    }).detach();
}

// ---------------------------------------------------------------------------
// Clear Caches (placeholder)
// ---------------------------------------------------------------------------
void clearAllCaches(EditorUI& ui) {
    LOG_WARN("Clear All Caches requested [Placeholder]");
    // TODO: implement cache clearing
}

// ---------------------------------------------------------------------------
// Draw Confirmation Popups
// ---------------------------------------------------------------------------
void drawModals(EditorUI& ui) {
    if (openDeletePopupNextFrame) {
        ImGui::OpenPopup("Confirm Delete?");
        openDeletePopupNextFrame = false;
    }

    if (!confirmDeletePopup) return;

    if (ImGui::BeginPopupModal("Confirm Delete?", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextWrapped("Delete %s:\n%s\n\nThis will permanently remove it.\nAre you sure?",
                           pendingDeleteType.c_str(),
                           pendingDeletePath.c_str());
        ImGui::Spacing();
        ImGui::Separator();

        if (ImGui::Button("Cancel", ImVec2(120, 0))) {
            confirmDeletePopup = false;
            pendingDeletePath.clear();
            pendingDeleteType.clear();
            ImGui::CloseCurrentPopup();
        }

        ImGui::SameLine();

        if (ImGui::Button("Delete", ImVec2(120, 0))) {
            try {
                std::error_code ec;
                size_t removed = fs::is_directory(pendingDeletePath)
                                 ? fs::remove_all(pendingDeletePath, ec)
                                 : (fs::remove(pendingDeletePath, ec) ? 1 : 0);

                if (ec)
                    LOG_ERROR("Failed to delete " + pendingDeleteType + ": " + ec.message());
                else
                    LOG_INFO("Deleted " + pendingDeleteType + ": " + pendingDeletePath +
                             " (" + std::to_string(removed) + " items)");

                ui.rescanAsyncPublic(ui.scannerMode());
                ui.filterAndSortTablesPublic();
            } catch (const std::exception& e) {
                LOG_ERROR(std::string("Exception deleting ") + pendingDeleteType + ": " + e.what());
            }

            confirmDeletePopup = false;
            pendingDeletePath.clear();
            pendingDeleteType.clear();
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}


}  // namespace menu_actions
