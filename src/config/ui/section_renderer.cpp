#include "section_renderer.h"
#include "section_config.h"
#include "config_ui.h"
#include <set>
#include <algorithm>
#include <map>

void SectionRenderer::render(const std::string& sectionName, nlohmann::json& sectionData, bool& isCapturing, std::string& capturingKeyName, ImGuiFileDialog* fileDialog, bool defaultOpen, bool& isDialogOpen, std::string& dialogKey) {
    SectionConfig config;
    std::string displayName = config.getSectionDisplayName(sectionName);
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_None;
    if (defaultOpen) {
        flags |= ImGuiTreeNodeFlags_DefaultOpen;
    }
    if (ImGui::CollapsingHeader(displayName.c_str(), flags)) {
        ImGui::Indent();

        // ----------------------- Special handling for AudioSettings section
        if (sectionName == "AudioSettings") {
            ImGui::Text("Audio Mixer");
            ImGui::Separator();

            // Draw circular "knobs" for volume levels (master/media/music/interface/ambience)
            auto knob = [](const char* label, float& value, float min, float max) {
                ImGui::BeginGroup();
                ImGui::Text("%s", label);
                float radius = 25.0f;
                ImVec2 center = ImGui::GetCursorScreenPos();
                center.x += radius;
                center.y += radius;

                // Create knob-like behavior
                ImGui::InvisibleButton(label, ImVec2(radius * 2, radius * 2));
                bool active = ImGui::IsItemActive();
                if (active)
                    value -= ImGui::GetIO().MouseDelta.y * 0.4f; // vertical drag

                value = std::clamp(value, min, max);

                // Visual feedback
                ImDrawList* draw = ImGui::GetWindowDrawList();
                draw->AddCircleFilled(center, radius, IM_COL32(40, 40, 40, 255));
                draw->AddCircle(center, radius, IM_COL32(180, 180, 180, 255));
                // Convert degrees to radians manually
                constexpr float PI = 3.14159265358979323846f;
                float angleDeg = (value - min) / (max - min) * 270.0f - 135.0f;
                float angleRad = angleDeg * PI / 180.0f;
                ImVec2 indicator(center.x + cosf(angleRad) * radius * 0.7f,
                                center.y + sinf(angleRad) * radius * 0.7f);

                draw->AddLine(center, indicator, IM_COL32(255, 200, 0, 255), 3.0f);

                ImGui::Dummy(ImVec2(radius * 2, radius * 2));
                ImGui::Text("%.0f", value);
                ImGui::EndGroup();
            };

            float masterVol = sectionData["masterVol"];
            float mediaVol = sectionData["mediaAudioVol"];
            float musicVol = sectionData["tableMusicVol"];
            float uiVol = sectionData["interfaceAudioVol"];
            float ambienceVol = sectionData["interfaceAmbienceVol"];

            ImGui::Columns(5, nullptr, false);
            knob("Master", masterVol, 0.0f, 100.0f); ImGui::NextColumn();
            knob("Media", mediaVol, 0.0f, 100.0f); ImGui::NextColumn();
            knob("Music", musicVol, 0.0f, 100.0f); ImGui::NextColumn();
            knob("UI", uiVol, 0.0f, 100.0f); ImGui::NextColumn();
            knob("Ambience", ambienceVol, 0.0f, 100.0f);
            ImGui::Columns(1);

            sectionData["masterVol"] = masterVol;
            sectionData["mediaAudioVol"] = mediaVol;
            sectionData["tableMusicVol"] = musicVol;
            sectionData["interfaceAudioVol"] = uiVol;
            sectionData["interfaceAmbienceVol"] = ambienceVol;

            // Toggle mutes below knobs
            ImGui::Separator();
            bool masterMute = sectionData["masterMute"];
            bool mediaMute = sectionData["mediaAudioMute"];
            bool musicMute = sectionData["tableMusicMute"];
            bool uiMute = sectionData["interfaceAudioMute"];
            bool ambienceMute = sectionData["interfaceAmbienceMute"];

            ImGui::Checkbox("Master Mute     ", &masterMute);
            ImGui::SameLine();
            ImGui::Checkbox("Media Mute      ", &mediaMute);
            ImGui::SameLine();
            ImGui::Checkbox("Music Mute      ", &musicMute);
            ImGui::SameLine();
            ImGui::Checkbox("UI Mute        ", &uiMute);
            ImGui::SameLine();
            ImGui::Checkbox("Ambience Mute", &ambienceMute);

            sectionData["masterMute"] = masterMute;
            sectionData["mediaAudioMute"] = mediaMute;
            sectionData["tableMusicMute"] = musicMute;
            sectionData["interfaceAudioMute"] = uiMute;
            sectionData["interfaceAmbienceMute"] = ambienceMute;

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Unindent();
            return; // skip generic rendering for this section
        }

        // ----------------------- Special handling for Window section
        if (sectionName == "WindowSettings") {
            ImGui::Text("Window Layout Preview");
            ImGui::Separator();

            // Only include windows that are toggled ON
            struct WindowData {
                const char* name;
                ImU32 color;
                const char* showKey;
                const char* xKey;
                const char* yKey;
                const char* wKey;
                const char* hKey;
                bool visible;
                int x, y, w, h;
            };

            WindowData windows[] = {
                {"Playfield", IM_COL32(80, 180, 255, 200), nullptr,
                    "playfieldX", "playfieldY", "playfieldWindowWidth", "playfieldWindowHeight",
                    true, 0, 0, 0, 0},
                {"Backglass", IM_COL32(255, 180, 80, 200), "showBackglass",
                    "backglassX", "backglassY", "backglassWindowWidth", "backglassWindowHeight",
                    sectionData.value("showBackglass", false), 0, 0, 0, 0},
                {"DMD", IM_COL32(180, 255, 100, 200), "showDMD",
                    "dmdX", "dmdY", "dmdWindowWidth", "dmdWindowHeight",
                    sectionData.value("showDMD", false), 0, 0, 0, 0},
                {"Topper", IM_COL32(255, 100, 200, 200), "showTopper",
                    "topperWindowX", "topperWindowY", "topperWindowWidth", "topperWindowHeight",
                    sectionData.value("showTopper", false), 0, 0, 0, 0},
            };

            // Calculate bounds and scaling for preview canvas
            int maxX = 0, maxY = 0;
            for (auto& w : windows) {
                if (!w.visible) continue;
                w.x = sectionData.value(w.xKey, 0);
                w.y = sectionData.value(w.yKey, 0);
                w.w = sectionData.value(w.wKey, 0);
                w.h = sectionData.value(w.hKey, 0);
                maxX = std::max(maxX, w.x + w.w);
                maxY = std::max(maxY, w.y + w.h);
            }
            float scale = 1.0f;
            if (maxX > 0 && maxY > 0)
                scale = std::min(400.0f / (float)maxX, 300.0f / (float)maxY);

            ImVec2 canvasSize(420, 320);
            ImVec2 canvasPos = ImGui::GetCursorScreenPos();
            ImDrawList* draw = ImGui::GetWindowDrawList();
            draw->AddRectFilled(canvasPos, ImVec2(canvasPos.x + canvasSize.x, canvasPos.y + canvasSize.y), IM_COL32(25, 25, 25, 255));
            draw->AddRect(canvasPos, ImVec2(canvasPos.x + canvasSize.x, canvasPos.y + canvasSize.y), IM_COL32(180, 180, 180, 255));

            // Handle dragging/resizing interaction
            ImGui::InvisibleButton("Canvas", canvasSize, ImGuiButtonFlags_MouseButtonLeft);
            bool isCanvasHovered = ImGui::IsItemHovered();
            ImVec2 mouse = ImGui::GetIO().MousePos;
            ImVec2 relMouse = ImVec2((mouse.x - canvasPos.x) / scale, (mouse.y - canvasPos.y) / scale);

            static int activeIndex = -1;
            static bool resizing = false;
            if (!ImGui::IsMouseDown(ImGuiMouseButton_Left))
                activeIndex = -1, resizing = false;

            for (int i = 0; i < IM_ARRAYSIZE(windows); i++) {
                auto& win = windows[i];
                if (!win.visible) continue;
                ImVec2 pMin(canvasPos.x + static_cast<float>(win.x) * scale,
                            canvasPos.y + static_cast<float>(win.y) * scale);
                ImVec2 pMax(canvasPos.x + static_cast<float>(win.x + win.w) * scale,
                            canvasPos.y + static_cast<float>(win.y + win.h) * scale);

                // Detect mouse hover
                bool hover = isCanvasHovered && mouse.x >= pMin.x && mouse.x <= pMax.x && mouse.y >= pMin.y && mouse.y <= pMax.y;
                ImU32 color = hover ? IM_COL32(255, 255, 255, 255) : IM_COL32(180, 180, 180, 255);
                draw->AddRectFilled(pMin, pMax, win.color);
                draw->AddRect(pMin, pMax, color);
                draw->AddText(ImVec2(pMin.x + 5, pMin.y + 5), IM_COL32(255, 255, 255, 255), win.name);

                // Start drag
                if (hover && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
                    activeIndex = i, resizing = ImGui::GetIO().KeyShift; // Shift = resize

                // Drag or resize
                if (i == activeIndex && ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
                    if (resizing) {
                        win.w = std::max(1, static_cast<int>(relMouse.x) - win.x);
                        win.h = std::max(1, static_cast<int>(relMouse.y) - win.y);
                    } else {
                        win.x = static_cast<int>(relMouse.x) - win.w / 2;
                        win.y = static_cast<int>(relMouse.y) - win.h / 2;
                    }
                    // Update JSON live
                    sectionData[win.xKey] = win.x;
                    sectionData[win.yKey] = win.y;
                    sectionData[win.wKey] = win.w;
                    sectionData[win.hKey] = win.h;
                }
            }

            //ImGui::Dummy(canvasSize);
            ImGui::TextDisabled("Tip: drag to move, Shift+drag to resize");
            ImGui::Spacing();
            ImGui::Separator();
        }


        // ----------------------- Generic rendering for other sections
        float singleFieldWidth = ImGui::GetContentRegionAvail().x * 0.5f;
        float pairedFieldWidth = ImGui::GetContentRegionAvail().x * 0.25f;

        const std::map<std::pair<std::string, std::string>, std::string> groupedKeys = {
            {{"playfieldWindowWidth", "playfieldWindowHeight"}, "Playfield Window Size [W,H]"},
            {{"playfieldX", "playfieldY"}, "Playfield Window Position [X,Y]"},
            {{"dmdWindowWidth", "dmdWindowHeight"}, "DMD Window Size [W,H]"},
            {{"dmdX", "dmdY"}, "DMD Window Position [X,Y]"},
            {{"backglassWindowWidth", "backglassWindowHeight"}, "Backglass Window Size [W,H]"},
            {{"backglassX", "backglassY"}, "Backglass Window Position [X,Y]"},
            {{"topperWindowWidth", "topperWindowHeight"}, "Topper Window Size [W,H]"},
            {{"topperWindowX", "topperWindowY"}, "Topper Window Position [X,Y]"},
            {{"titleX", "titleY"}, "Title Position [X,Y]"},
            {{"arrowHintWidth", "arrowHintHeight"}, "Arrow Widget Size [W,H]"},
            {{"metadataPanelWidth", "metadataPanelHeight"}, "Metadata Panel Size [W,H]"},
            {{"wheelMediaWidth", "wheelMediaHeight"}, "Wheel Media Size [W,H]"},
            {{"wheelMediaX", "wheelMediaY"}, "Wheel Media Position [X,Y]"},
            {{"playfieldMediaWidth", "playfieldMediaHeight"}, "Playfield Media Size [W,H]"},
            {{"playfieldMediaX", "playfieldMediaY"}, "Playfield Media Position [X,Y]"},
            {{"backglassMediaWidth", "backglassMediaHeight"}, "Backglass Media Size [W,H]"},
            {{"backglassMediaX", "backglassMediaY"}, "Backglass Media Position [X,Y]"},
            {{"dmdMediaWidth", "dmdMediaHeight"}, "DMD Media Size [W,H]"},
            {{"dmdMediaX", "dmdMediaY"}, "DMD Media Position [X,Y]"},
            {{"topperMediaWidth", "topperMediaHeight"}, "Topper Media Size [W,H]"},
            {{"topperMediaX", "topperMediaY"}, "Topper Window Position [X,Y]"},
            {{"configUIWidth", "configUIHeight"}, "Config Window Size [W,H]"}
        };
        std::set<std::string> processedKeys;

        std::set<std::string> orderedSet(orderedKeys_.begin(), orderedKeys_.end());
        for (const auto& key : orderedKeys_) {
            if (!sectionData.contains(key)) continue;
            ImGui::PushID(key.c_str());
            auto& value = sectionData[key];
            std::string keyDisplayName = config.getKeyDisplayName(sectionName, key);
            ImGui::PushItemWidth(singleFieldWidth);

            for (const auto& [keyPair, groupLabel] : groupedKeys) {
                const auto& [key1, key2] = keyPair;
                if (key == key1 && sectionData.contains(key2) && !processedKeys.count(key1)) {
                    ImGui::PushItemWidth(pairedFieldWidth);
                    if (value.is_number_integer()) {
                        int val = value.get<int>();
                        if (ImGui::InputInt("##first", &val)) {
                            value = val;
                        }
                    } else if (value.is_number_float()) {
                        float val = value.get<float>();
                        if (ImGui::InputFloat("##first", &val)) {
                            value = val;
                        }
                    } else {
                        LOG_DEBUG("Skipping invalid type for " + std::string(key1) + ", expected number, got " + std::string(value.type_name()));
                        int val = 0;
                        ImGui::InputInt("##first", &val);
                    }
                    ImGui::SameLine();
                    auto& value2 = sectionData[key2];
                    if (value2.is_number_integer()) {
                        int val = value2.get<int>();
                        if (ImGui::InputInt("##second", &val)) {
                            value2 = val;
                        }
                    } else if (value2.is_number_float()) {
                        float val = value2.get<float>();
                        if (ImGui::InputFloat("##second", &val)) {
                            value2 = val;
                        }
                    } else {
                        LOG_DEBUG("Skipping invalid type for " + std::string(key2) + ", expected number, got " + std::string(value2.type_name()));
                        int val = 0;
                        ImGui::InputInt("##second", &val);
                    }
                    ImGui::SameLine();
                    ImGui::Text("%s", groupLabel.c_str());
                    ImGui::PopItemWidth();
                    processedKeys.insert(key1);
                    processedKeys.insert(key2);
                    //LOG_DEBUG("Rendered group " << groupLabel << " with keys " << key1 << ", " << key2);
                    break;
                }
            }

            if (!processedKeys.count(key)) {
                if (value.is_boolean()) {
                    renderBool(keyDisplayName, value, sectionName);
                } else if (value.is_number_float()) {
                    float minVal = (key.find("Alpha") != std::string::npos || key.find("Scale") != std::string::npos) ? 0.0f : 0.0f;
                    float maxVal = (key.find("Alpha") != std::string::npos || key.find("Scale") != std::string::npos) ? 1.0f : 100.0f;
                    renderFloat(keyDisplayName, value, sectionName, minVal, maxVal);
                } else if (value.is_number_integer()) {
                    if (key.find("Rotation") != std::string::npos) {
                        renderRotation(keyDisplayName, value, sectionName);
                    } else {
                        renderInt(keyDisplayName, value, sectionName);
                    }
                } else if (value.is_string()) {
                    const auto& options = config.getDropdownOptions(sectionName, key);
                    if (!options.empty()) {
                        std::string val = value.get<std::string>();
                        int currentIndex = -1;
                        for (size_t i = 0; i < options.size(); ++i) {
                            if (options[i] == val) {
                                currentIndex = static_cast<int>(i);
                                break;
                            }
                        }
                        if (ImGui::Combo(keyDisplayName.c_str(), &currentIndex, [](void* data, int idx, const char** out_text) {
                            auto& localOptions = *static_cast<std::vector<std::string>*>(data);
                            if (idx >= 0 && idx < static_cast<int>(localOptions.size())) {
                                *out_text = localOptions[idx].c_str();
                                return true;
                            }
                            return false;
                        }, (void*)&options, static_cast<int>(options.size()))) {
                            if (currentIndex >= 0 && currentIndex < static_cast<int>(options.size())) {
                                value = options[currentIndex];
                                LOG_DEBUG("Updated " + std::string(keyDisplayName) + " to " + std::string(options[currentIndex]));
                            }
                        }
                    } else if (sectionName == "VPX" && (key == "VPXTablesPath" || key == "VPinballXPath" || key == "vpxIniPath")) {
                        //LOG_DEBUG("Rendering path for key " << key << " in section " << sectionName);
                        renderPathOrExecutable(key, value, sectionName, fileDialog, isDialogOpen, dialogKey);
                    } else if (sectionName == "UISounds" || sectionName == "DefaultMedia" || sectionName == "CustomMedia") {
                        renderPathOrExecutable(key, value, sectionName, fileDialog, isDialogOpen, dialogKey);
                    } else if (sectionName == "UIWidgets" && key == "fontPath") {
                        renderPathOrExecutable(key, value, sectionName, fileDialog, isDialogOpen, dialogKey);
                    } else if (sectionName == "Keybinds") {
                        renderKeybind(keyDisplayName, value, sectionName, isCapturing, capturingKeyName);
                    } else {
                        renderString(keyDisplayName, value, sectionName);
                    }
                } else if (value.is_array() && value.size() == 4) {
                    renderColor(keyDisplayName, value, sectionName);
                } else {
                    LOG_DEBUG("Skipping invalid type for " + std::string(keyDisplayName) + ", expected valid type, got " + std::string(value.type_name()));
                    int val = 0;
                    ImGui::InputInt(keyDisplayName.c_str(), &val);
                }
                if (ImGui::IsItemHovered()) {
                    auto it = Settings::settingsMetadata.find(key);
                    if (it != Settings::settingsMetadata.end()) {
                        ImGui::BeginTooltip();
                        ImGui::TextUnformatted(it->second.second.c_str());
                        ImGui::EndTooltip();
                    }
                }
            }
            ImGui::PopItemWidth();
            ImGui::PopID();
        }

        std::vector<std::string> remainingKeys;
        for (const auto& [key, _] : sectionData.items()) {
            if (!orderedSet.count(key) && !processedKeys.count(key)) remainingKeys.push_back(key);
        }
        std::sort(remainingKeys.begin(), remainingKeys.end());

        for (const auto& key : remainingKeys) {
            ImGui::PushID(key.c_str());
            auto& value = sectionData[key];
            std::string keyDisplayName = config.getKeyDisplayName(sectionName, key);
            ImGui::PushItemWidth(singleFieldWidth);

            if (value.is_boolean()) {
                renderBool(keyDisplayName, value, sectionName);
            } else if (value.is_number_float()) {
                float minVal = (key.find("Alpha") != std::string::npos || key.find("Scale") != std::string::npos) ? 0.0f : 0.0f;
                float maxVal = (key.find("Alpha") != std::string::npos || key.find("Scale") != std::string::npos) ? 1.0f : 100.0f;
                renderFloat(keyDisplayName, value, sectionName, minVal, maxVal);
            } else if (value.is_number_integer()) {
                if (key.find("Rotation") != std::string::npos) {
                    renderRotation(keyDisplayName, value, sectionName);
                } else {
                    renderInt(keyDisplayName, value, sectionName);
                }
            } else if (value.is_string()) {
                const auto& options = config.getDropdownOptions(sectionName, key);
                if (!options.empty()) {
                    std::string val = value.get<std::string>();
                    int currentIndex = -1;
                    for (size_t i = 0; i < options.size(); ++i) {
                        if (options[i] == val) {
                            currentIndex = static_cast<int>(i);
                            break;
                        }
                    }
                    if (ImGui::Combo(keyDisplayName.c_str(), &currentIndex, [](void* data, int idx, const char** out_text) {
                        auto& localOptions = *static_cast<std::vector<std::string>*>(data);
                        if (idx >= 0 && idx < static_cast<int>(localOptions.size())) {
                            *out_text = localOptions[idx].c_str();
                            return true;
                        }
                        return false;
                    }, (void*)&options, static_cast<int>(options.size()))) {
                        if (currentIndex >= 0 && currentIndex < static_cast<int>(options.size())) {
                            value = options[currentIndex];
                            LOG_DEBUG("Updated " + std::string(keyDisplayName) + " to " + std::string(options[currentIndex]));
                        }
                    }
                } else if (sectionName == "VPX" && (key == "VPXTablesPath" || key == "VPinballXPath" || key == "vpxIniPath")) {
                    //LOG_DEBUG("Rendering path for key " << key << " in section " << sectionName);
                    renderPathOrExecutable(key, value, sectionName, fileDialog, isDialogOpen, dialogKey);
                } else if (sectionName == "UIWidgets" && key == "fontPath") {
                    renderPathOrExecutable(key, value, sectionName, fileDialog, isDialogOpen, dialogKey);
                } else if (sectionName == "Keybinds") {
                    renderKeybind(keyDisplayName, value, sectionName, isCapturing, capturingKeyName);
                } else {
                    renderString(keyDisplayName, value, sectionName);
                }
            } else if (value.is_array() && value.size() == 4) {
                renderColor(keyDisplayName, value, sectionName);
            } else {
                LOG_DEBUG("Skipping invalid type for " + std::string(keyDisplayName) + ", expected valid type, got " + std::string(value.type_name()));
                int val = 0;
                ImGui::InputInt(keyDisplayName.c_str(), &val);
            }
            if (ImGui::IsItemHovered()) {
                auto it = Settings::settingsMetadata.find(key);
                if (it != Settings::settingsMetadata.end()) {
                    ImGui::BeginTooltip();
                    ImGui::TextUnformatted(it->second.second.c_str());
                    ImGui::EndTooltip();
                }
            }
            ImGui::PopItemWidth();
            ImGui::PopID();
        }
        if (!configUI_->isStandalone()) {
            ImGui::PushID("ResetButton");
            if (ImGui::Button("Reset to Defaults", ImVec2(130, 0))) {
                if (configUI_) {  // Check that the pointer is valid
                    configUI_->resetSectionToDefault(sectionName);
                }
            }
            ImGui::PopID();
        }
        ImGui::Unindent();
    }
}
