#include "editor/editor_ui.h"
#include "log/logger.h"
#include "config/settings.h"
#include <thread>

EditorUI::EditorUI(IConfigService* config, ITableLoader* tableLoader)
    : config_(config), tableLoader_(tableLoader) {
    Settings settings = config_->getSettings();
    // Always read from existing index file when loading editor mode
    settings.ignoreScanners = true;
    tables_ = tableLoader_->loadTableList(settings, nullptr);
}

void EditorUI::draw() {
    ImGui::Begin("ASAPCabinetFE Table Editor");

    if (loading_) {
        ImGui::Text("Rescanning tables...");
        ImGui::End();
        return;
    }

    std::lock_guard<std::mutex> lock(tableMutex_);

    if (tables_.empty()) {
        ImGui::TextDisabled("No tables found. Verify configuration or rescan in main frontend.");
    } else if (ImGui::BeginTable("tableList", 3, ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable)) {
        ImGui::TableSetupColumn("Year");
        ImGui::TableSetupColumn("Table Name");
        ImGui::TableSetupColumn("Path");
        ImGui::TableHeadersRow();

        for (const auto& table : tables_) {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(table.year.c_str());
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(table.title.c_str());
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(table.vpxFile.c_str());
        }
        ImGui::EndTable();
    }

    if (ImGui::Button("Rescan Tables")) {
        rescanAsync();
    }

    ImGui::SameLine();
    if (ImGui::Button("Exit Editor")) {
        exitRequested_ = true;
    }

    ImGui::End();
}

void EditorUI::rescanAsync() {
    if (loading_) return;
    loading_ = true;

    std::thread([this]() {
        Settings settings = config_->getSettings();
        auto newTables = tableLoader_->loadTableList(settings, nullptr);
        {
            std::lock_guard<std::mutex> lock(tableMutex_);
            tables_ = std::move(newTables);
        }
        loading_ = false;
    }).detach();
}
