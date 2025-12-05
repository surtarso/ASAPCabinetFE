#include "table_override_editor.h"
#include "imgui.h"
#include "log/logging.h"
#include <array>
// TableOverrideEditor Customization Settings (hardcoded for future ConfigUI integration)
// namespace TableOverrideEditorSettings
// {
//     constexpr float WIDTH_FACTOR = 0.7f;   // 70% of screen width
//     constexpr float HEIGHT_FACTOR = 0.52f; // 50% of screen height
//     constexpr float ALPHA = 0.8f;          // Semi-transparent background
// }
TableOverrideEditor::TableOverrideEditor(TableData &table, TableOverrideManager &overrideManager)
    : table_(table), overrideManager_(overrideManager), shouldClose_(false), saved_(false)
{
    // Initialize fields with current TableData values (limited to overrideable ones)
    fields_ = {
        {"bestTitle", table_.bestTitle},
        {"bestManufacturer", table_.bestManufacturer},
        {"bestYear", table_.bestYear},
        {"vpsId", table_.vpsId}
    };

    originalFields_ = fields_; // Store for change detection
    if (overrideManager_.overrideFileExists(table)) {
        // This means current vpsId is MANUAL → original was whatever was there before override
        // We don't know it → safest: assume it was EMPTY (most common case)
        originalFields_["vpsId"] = "";  // ← CRITICAL LINE
    }
    LOG_DEBUG("Initialized for table: " + table_.bestTitle);
}
bool TableOverrideEditor::render()
{
    if (shouldClose_)
    {
        return false;
    }
    // Center and fix window size
    ImGuiIO &io = ImGui::GetIO();

    ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(io.DisplaySize.x, io.DisplaySize.y), ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.8f);

    ImGui::Begin("ASAPCabinetFE Override Editor", nullptr,
                 ImGuiWindowFlags_NoMove |
                     ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);

    // Explanatory feedback text
    ImGui::Text("Override these fields to customize display and sorting in the frontend.");
    ImGui::Text("bestTitle, bestManufacturer, and bestYear affect how the table is shown and sorted.");
    ImGui::Text("vpsId allows manual specification for VPSDB matching during scans.");
    ImGui::Text("This will be used to matchmake correctly without relying on automatic detection.");
    ImGui::Separator();
    // Persistent buffers for text fields
    static std::map<std::string, std::array<char, 1024>> buffers;
    // Initialize or update buffers
    for (const auto &[key, value] : fields_)
    {
        if (buffers.find(key) == buffers.end() || std::string(buffers[key].data()) != value)
        {
            buffers[key].fill(0);
            strncpy(buffers[key].data(), value.c_str(), buffers[key].size() - 1);
        }
    }
    // Render text fields in two columns
    ImGui::Columns(2, "Fields", false);
    float keyWidth = ImGui::CalcTextSize("bestManufacturer").x + ImGui::GetStyle().FramePadding.x * 2; // Longest key
    ImGui::SetColumnWidth(0, keyWidth);
    for (const auto &key : {"bestTitle", "bestManufacturer", "bestYear", "vpsId"})
    {
        ImGui::Text("%s", key);
        ImGui::NextColumn();
        ImGui::PushID(key);
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - ImGui::GetStyle().FramePadding.x * 2);
        ImGui::InputText("##field", buffers[key].data(), buffers[key].size());
        if (ImGui::IsItemDeactivatedAfterEdit())
        {
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
    // Render buttons
    bool hasChanges = fields_ != originalFields_;
    if (hasChanges)
    {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.8f, 0.2f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.9f, 0.3f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.1f, 0.7f, 0.1f, 1.0f));
    }
    if (ImGui::Button("Save", ImVec2(100, 0)))
    {
        save();
        shouldClose_ = true;
    }
    ImGui::SameLine();
    if (ImGui::Button("Discard", ImVec2(100, 0)))
    {
        shouldClose_ = true;
        LOG_DEBUG("Discarded changes for table: " + table_.bestTitle);
    }
    if (hasChanges)
    {
        ImGui::PopStyleColor(3);
    }
    ImGui::End();
    return !shouldClose_;
}

void TableOverrideEditor::save()
{
    std::map<std::string, std::string> overrides;
    bool vpsIdTouched = false;

    for (const auto& [key, value] : fields_) {
        if (value != originalFields_[key]) {
            overrides[key] = value;
            if (key == "vpsId") vpsIdTouched = true;
        }
    }

    if (!overrides.empty()) {
        // User changed something → save override
        if (vpsIdTouched) {
            table_.vpsId = fields_["vpsId"];
            table_.isManualVpsId = !fields_["vpsId"].empty();
        }
        overrideManager_.saveOverride(table_, overrides);
        saved_ = true;
    } else if (overrideManager_.overrideFileExists(table_)) {
        table_.vpsId.clear();                     // ← force clear (let's the app try to rematch)
        table_.isManualVpsId = false;             // ← definitely not manual
        overrideManager_.deleteOverride(table_);  // deleteOverride will save tables
        LOG_DEBUG("Override deleted → vpsId cleared");
    }
}
