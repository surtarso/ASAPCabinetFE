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
// Compression Request (placeholder)
// ---------------------------------------------------------------------------
void requestCompressTableFolder(EditorUI& ui) {
    LOG_WARN("Compress Table Folder requested [Placeholder]");
    // TODO: implement zip thread
}

// ---------------------------------------------------------------------------
// Repair Table (placeholder)
// ---------------------------------------------------------------------------
void repairTableViaVpxtool(EditorUI& ui) {
    LOG_WARN("Repair Table via VPXTool requested [Placeholder]");
    // TODO: spawn vpxtool subprocess if found in PATH
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
