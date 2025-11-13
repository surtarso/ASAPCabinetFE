/**
 * @file table_override_editor.cpp
 * @brief Implements the TableOverrideEditor class for editing table overrides in ASAPCabinetFE.
 *
 * This file provides the implementation of the TableOverrideEditor class, which renders an
 * ImGui panel with text fields for overrideable TableData fields. Users can edit fields, save
 * changes to <table_name>.json, or discard changes. The panel is centered, unmovable, unresizable,
 * and matches the visual style of the metadata panel and config UI.
 */

#include "table_override_editor.h"
#include "log/logging.h"
#include "imgui.h"
#include <vector>
#include <array>

// TableOverrideEditor Customization Settings (hardcoded for future ConfigUI integration)
namespace TableOverrideEditorSettings {
    constexpr float WIDTH_FACTOR = 0.7f;  // 70% of screen width
    constexpr float HEIGHT_FACTOR = 0.52f; // 50% of screen height
    constexpr float ALPHA = 0.8f;        // Semi-transparent background
}

TableOverrideEditor::TableOverrideEditor(TableData& table, TableOverrideManager& overrideManager)
    : table_(table), overrideManager_(overrideManager), shouldClose_(false), saved_(false) {
    // Initialize fields with current TableData values
    fields_ = {
        {"title", table_.title},
        {"manufacturer", table_.manufacturer},
        {"year", table_.year},
        {"vpxFile", table_.vpxFile},
        {"folder", table_.folder},
        {"playfieldImage", table_.playfieldImage},
        {"wheelImage", table_.wheelImage},
        {"backglassImage", table_.backglassImage},
        {"dmdImage", table_.dmdImage},
        {"topperImage", table_.topperImage},
        {"playfieldVideo", table_.playfieldVideo},
        {"backglassVideo", table_.backglassVideo},
        {"dmdVideo", table_.dmdVideo},
        {"topperVideo", table_.topperVideo},
        {"music", table_.music},
        {"launchAudio", table_.launchAudio},
        {"romPath", table_.romPath},
        {"romName", table_.romName},
        {"hasAltSound", table_.hasAltSound ? "true" : "false"},
        {"hasAltColor", table_.hasAltColor ? "true" : "false"},
        {"hasPup", table_.hasPup ? "true" : "false"},
        {"hasAltMusic", table_.hasAltMusic ? "true" : "false"},
        {"hasUltraDMD", table_.hasUltraDMD ? "true" : "false"},
        {"tableName", table_.tableName},
        {"tableAuthor", table_.tableAuthor},
        {"tableDescription", table_.tableDescription},
        {"tableSaveDate", table_.tableSaveDate},
        {"tableLastModified", table_.tableLastModified},
        {"tableReleaseDate", table_.tableReleaseDate},
        {"tableVersion", table_.tableVersion},
        {"tableRevision", table_.tableRevision},
        {"tableBlurb", table_.tableBlurb},
        {"tableRules", table_.tableRules},
        {"tableAuthorEmail", table_.tableAuthorEmail},
        {"tableAuthorWebsite", table_.tableAuthorWebsite},
        {"tableType", table_.tableType},
        {"tableManufacturer", table_.tableManufacturer},
        {"tableYear", table_.tableYear}
    };
    originalFields_ = fields_; // Store for reset
    LOG_DEBUG("Initialized for table: " + table_.title + " with " + std::to_string(fields_.size()) + " fields");
}

bool TableOverrideEditor::render() {
    if (shouldClose_) {
        return false;
    }

    // Center and fix window size
    ImGuiIO& io = ImGui::GetIO();

    // Detect display orientation
    bool isLandscape = io.DisplaySize.x > io.DisplaySize.y;

    // Adjust window layout depending on orientation
    if (isLandscape) {
        // Landscape → full-screen mode
        ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(io.DisplaySize.x, io.DisplaySize.y), ImGuiCond_Always);
        ImGui::SetNextWindowBgAlpha(TableOverrideEditorSettings::ALPHA);
    } else {
        // Portrait → keep centered floating layout
        float panelWidth = io.DisplaySize.x * TableOverrideEditorSettings::WIDTH_FACTOR;
        float panelHeight = io.DisplaySize.y * TableOverrideEditorSettings::HEIGHT_FACTOR;
        float posX = (io.DisplaySize.x - panelWidth) / 2.0f;
        float posY = (io.DisplaySize.y - panelHeight) / 2.0f;

        ImGui::SetNextWindowPos(ImVec2(posX, posY), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(panelWidth, panelHeight), ImGuiCond_Always);
        ImGui::SetNextWindowBgAlpha(TableOverrideEditorSettings::ALPHA);
    }

    ImGui::Begin("ASAPCabinetFE Metadata Editor", nullptr,
                 ImGuiWindowFlags_NoMove |
                 ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);

    // Persistent buffers for text fields
    static std::map<std::string, std::array<char, 1024>> buffers;

    // Initialize or update buffers
    for (const auto& [key, value] : fields_) {
        if (buffers.find(key) == buffers.end() || std::string(buffers[key].data()) != value) {
            buffers[key].fill(0);
            strncpy(buffers[key].data(), value.c_str(), buffers[key].size() - 1);
        }
    }

    // Render text fields in two columns
    ImGui::Columns(2, "Fields", false);
    float keyWidth = ImGui::CalcTextSize("tableAuthorWebsite").x + ImGui::GetStyle().FramePadding.x * 2; // Longest key
    ImGui::SetColumnWidth(0, keyWidth);

    // FILE SCANNER section
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "FILE SCANNER");
    ImGui::Separator();
    for (const auto& key : {"title", "manufacturer", "year", "vpxFile", "folder", "playfieldImage", "wheelImage",
                           "backglassImage", "dmdImage", "topperImage", "playfieldVideo", "backglassVideo",
                           "dmdVideo", "topperVideo", "music", "launchAudio", "romPath", "romName", "hasAltSound",
                           "hasAltColor", "hasPup", "hasAltMusic", "hasUltraDMD"}) {
        ImGui::Text("%s", key);
        ImGui::NextColumn();

        ImGui::PushID(key);
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - ImGui::GetStyle().FramePadding.x * 2);
        ImGui::InputText("##field", buffers[key].data(), buffers[key].size());

        if (ImGui::IsItemDeactivatedAfterEdit()) {
            fields_[key] = buffers[key].data();
            LOG_DEBUG("Updated field " + std::string(key) + " to: " + std::string(fields_[key]));
        }
        ImGui::PopID();
        ImGui::NextColumn();
    }

    // FILE METADATA section
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "FILE METADATA");
    ImGui::Separator();
    for (const auto& key : {"tableName", "tableAuthor", "tableDescription", "tableSaveDate", "tableLastModified",
                           "tableReleaseDate", "tableVersion", "tableRevision", "tableBlurb", "tableRules",
                           "tableAuthorEmail", "tableAuthorWebsite", "tableType", "tableManufacturer", "tableYear"}) {
        ImGui::Text("%s", key);
        ImGui::NextColumn();

        ImGui::PushID(key);
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - ImGui::GetStyle().FramePadding.x * 2);
        ImGui::InputText("##field", buffers[key].data(), buffers[key].size());

        if (ImGui::IsItemDeactivatedAfterEdit()) {
            fields_[key] = buffers[key].data();
            LOG_DEBUG("Updated field " + std::string(key) + " to: " + std::string(fields_[key]));
        }
        ImGui::PopID();
        ImGui::NextColumn();
    }

    ImGui::Columns(1); // End columns

    // Reserve space for buttons
    float buttonHeight = ImGui::GetFrameHeightWithSpacing() + 15.0f;
    ImGui::BeginChild("EditorContent", ImVec2(0, -buttonHeight), false);
    ImGui::EndChild();

    // ImGui::Separator();
    // ImGui::SetCursorPosY(ImGui::GetWindowHeight() - buttonHeight);

    // Render buttons
    bool hasChanges = fields_ != originalFields_;
    if (hasChanges) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.8f, 0.2f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.9f, 0.3f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.1f, 0.7f, 0.1f, 1.0f));
    }
    if (ImGui::Button("Save", ImVec2(100, 0))) {
        save();
        shouldClose_ = true;
    }
    ImGui::SameLine();
    if (ImGui::Button("Discard", ImVec2(100, 0))) {
        shouldClose_ = true;
        LOG_DEBUG("Discarded changes for table: " + table_.title);
    }
    if (hasChanges) {
        ImGui::PopStyleColor(3);
    }

    ImGui::End();
    return !shouldClose_;
}

void TableOverrideEditor::save() {
    std::map<std::string, std::string> overrides;
    for (const auto& [key, value] : fields_) {
        if (value != originalFields_[key]) {
            overrides[key] = value;
        }
    }
    if (!overrides.empty()) {
        overrideManager_.saveOverride(table_, overrides);
        saved_ = true;
        LOG_INFO("Saved overrides for table: " + table_.title);
    } else {
        saved_ = false;
        if (overrideManager_.overrideFileExists(table_)) {
            overrideManager_.deleteOverride(table_);
            LOG_DEBUG("Deleted empty override file for table: " + table_.title);
        } else {
            LOG_DEBUG("No changes to save and no override file exists for table: " + table_.title);
        }
    }
}
