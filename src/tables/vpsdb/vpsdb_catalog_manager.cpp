#include "vpsdb_catalog_manager.h"
#include "vpsdb_catalog_table.h"
#include "log/logging.h"
#include <nlohmann/json.hpp>
#include <filesystem>
#include <fstream>
#include <thread>
#include <cstdlib>

namespace fs = std::filesystem;

namespace vpsdb {

VpsdbCatalog::VpsdbCatalog(SDL_Renderer* renderer, const Settings& settings, VpsdbJsonLoader& jsonLoader)
    : renderer_(renderer),
      currentIndex_(0),
      isTableLoading_(false),
      isOpen(false),
      backglassTexture_(nullptr, [](SDL_Texture*){}),
      playfieldTexture_(nullptr, [](SDL_Texture*){}),
      settings_(settings),
      jsonLoader_(jsonLoader) {
}

VpsdbCatalog::~VpsdbCatalog() {
    if (tableLoadThread_.joinable()) {
        tableLoadThread_.join();
    }

    // Free cached textures
    for (auto& [path, tex] : textureCache_) {
        SDL_DestroyTexture(tex);
    }
    textureCache_.clear();

    VpsdbImage::clearThumbnails(*this);
}

void VpsdbCatalog::startTableLoad(size_t index, const std::string& vpsdbImageCacheDir) {
    if (tableLoadThread_.joinable()) {
        LOG_DEBUG("Joining existing thread before starting new load for index: " + std::to_string(index));
        tableLoadThread_.join();
    }

    jsonLoader_.waitForInit();

    // 1. Get the already loaded JSON (by const reference)
    const nlohmann::json& fullJson = jsonLoader_.getVpsDb();

    LOG_DEBUG("Starting table load for index: " + std::to_string(index));
    isTableLoading_ = true;

    // 2. Pass the JSON object BY VALUE (copy) to the thread for safety.
    //    Remove std::ref since we are passing a copy.
    tableLoadThread_ = std::thread(vpsdb::loadTableInBackground,
                                   fullJson, // <-- Pass by value (copy)
                                   index,
                                   std::ref(loadedTableQueue_),
                                   std::ref(mutex_),
                                   std::ref(isTableLoading_),
                                   vpsdbImageCacheDir);
    LOG_DEBUG("Thread created for index: " + std::to_string(index));
}

bool VpsdbCatalog::render() {
    if (jsonLoader_.isLoading()) {
        ImGuiIO& io = ImGui::GetIO();
        ImGui::SetNextWindowPos(ImVec2((io.DisplaySize.x - 300) / 2, (io.DisplaySize.y - 100) / 2), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(300, 100), ImGuiCond_Always);
        ImGui::Begin("Loading VPSDB", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar);

        const char* textToDisplay = "";
        switch (jsonLoader_.getProgressStage()) {
            case 1: textToDisplay = "Fetching VPSDB..."; break;
            case 2: textToDisplay = "Loading JSON..."; break;
            default: textToDisplay = "Loading VPSDB..."; break;
        }
        ImVec2 textSize = ImGui::CalcTextSize(textToDisplay);
        ImGui::SetCursorPosX((300 - textSize.x) / 2);
        ImGui::SetCursorPosY((100 - textSize.y) / 2);
        ImGui::Text("%s", textToDisplay);
        ImGui::End();
        return true;
    }

    jsonLoader_.initialize();
    // LOG_DEBUG("JSON loader initialized, loaded: " << jsonLoader_.isLoaded() << ", index size: " << jsonLoader_.getIndex().size());

    if (!jsonLoader_.isLoaded()) {
        if (!jsonLoader_.isLoading() && jsonLoader_.getIndex().empty()) {
            ImGui::Text("Error: VPSDB JSON not loaded");
            LOG_ERROR("JSON not loaded at " + settings_.vpsDbPath);
        }
        return true;
    }

    // ImGuiIO& io = ImGui::GetIO();
    // float panelWidth = io.DisplaySize.x * 0.7f;
    // float panelHeight = io.DisplaySize.y * 0.52f;
    // float posX = (io.DisplaySize.x - panelWidth) / 2.0f;
    // float posY = (io.DisplaySize.y - panelHeight) / 2.0f;

    // ImGui::SetNextWindowPos(ImVec2(posX, posY), ImGuiCond_Always);
    // ImGui::SetNextWindowSize(ImVec2(panelWidth, panelHeight), ImGuiCond_Always);
    // ImGui::SetNextWindowBgAlpha(0.8f);

    // ImGui::Begin("VPSDB Catalog", nullptr,
    //              ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);

    float panelWidth = 0.0f;
    float panelHeight = 0.0f;
    float posX = 0.0f;
    float posY = 0.0f;

    ImGuiIO& io = ImGui::GetIO();
    ImGuiWindowFlags windowFlags;

    // Smart detection based on aspect ratio
    if (io.DisplaySize.x > io.DisplaySize.y) {
        // --- LANDSCAPE (Editor App) ---
        // Use full viewport, no title bar, opaque background
        const ImGuiViewport* viewport = ImGui::GetMainViewport();

        posX = viewport->WorkPos.x;
        posY = viewport->WorkPos.y;
        panelWidth = viewport->WorkSize.x;
        panelHeight = viewport->WorkSize.y;

        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(viewport->WorkSize);
        ImGui::SetNextWindowBgAlpha(1.0f); // Opaque

        windowFlags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
                    ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar;

    } else {
        // --- PORTRAIT (Main App) ---
        panelWidth = io.DisplaySize.x * 0.7f;
        panelHeight = io.DisplaySize.y * 0.52f;
        posX = (io.DisplaySize.x - panelWidth) / 2.0f;
        posY = (io.DisplaySize.y - panelHeight) / 2.0f;

        ImGui::SetNextWindowPos(ImVec2(posX, posY), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(panelWidth, panelHeight), ImGuiCond_Always);
        ImGui::SetNextWindowBgAlpha(0.8f); // Semi-transparent

        windowFlags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
                    ImGuiWindowFlags_NoCollapse;
    }

    ImGui::Begin("VPSDB Catalog", nullptr, windowFlags);

    // Process loaded table data
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!loadedTableQueue_.empty()) {
            auto data = std::move(loadedTableQueue_.front());
            loadedTableQueue_.pop();
            LOG_DEBUG("Processing queued table, index: " + std::to_string(data.index));

            currentIndex_ = data.index;
            currentTable_ = std::move(data.table);
            VpsdbImage::clearThumbnails(*this);

            // --- BACKGLASS ---
            if (!data.backglassPath.empty()) {

                // Check memory cache first
                auto it = textureCache_.find(data.backglassPath);
                if (it != textureCache_.end()) {
                    // Already in memory = instant load
                    backglassTexture_.reset(it->second);
                    LOG_DEBUG("Using cached backglass texture: " + data.backglassPath);
                } else {
                    // Not cached = load from disk
                    SDL_Texture* tex = VpsdbImage::loadTexture(*this, data.backglassPath);
                    if (tex) {
                        textureCache_[data.backglassPath] = tex; // ADD TO CACHE
                        backglassTexture_.reset(tex);
                        LOG_DEBUG("Loaded and cached backglass texture: " + data.backglassPath);
                    }
                }

                currentBackglassPath_ = data.backglassPath;
            }

            // --- PLAYFIELD ---
            if (!data.playfieldPath.empty()) {

                auto it = textureCache_.find(data.playfieldPath);
                if (it != textureCache_.end()) {
                    playfieldTexture_.reset(it->second);
                    LOG_DEBUG("Using cached playfield texture: " + data.playfieldPath);
                } else {
                    SDL_Texture* tex = VpsdbImage::loadTexture(*this, data.playfieldPath);
                    if (tex) {
                        textureCache_[data.playfieldPath] = tex;
                        playfieldTexture_.reset(tex);
                        LOG_DEBUG("Loaded and cached playfield texture: " + data.playfieldPath);
                    }
                }

                currentPlayfieldPath_ = data.playfieldPath;
            }


            isTableLoading_ = false;
            LOG_DEBUG("Processed loaded table, index: " + std::to_string(currentIndex_));
        }
        // else {
        //     LOG_DEBUG("Loaded table queue is empty");
        // }
    }

    // Search input and button
    static char searchBuffer[256] = "";
    ImGui::InputText("##Search", searchBuffer, IM_ARRAYSIZE(searchBuffer), ImGuiInputTextFlags_EnterReturnsTrue);
    ImGui::SameLine();
    if (ImGui::Button("Fetch") || ImGui::IsKeyPressed(ImGuiKey_Enter, true)) {
        applySearchFilter(searchBuffer);
    }
    if (strlen(searchBuffer) > 0) {
        std::string searchStr = searchBuffer;
        std::transform(searchStr.begin(), searchStr.end(), searchStr.begin(), ::tolower);
        std::vector<size_t> tempIndices;
        for (size_t i = 0; i < jsonLoader_.getIndex().size(); ++i) {
            std::string name = jsonLoader_.getIndex()[i].name;
            std::transform(name.begin(), name.end(), name.begin(), ::tolower);
            if (name.find(searchStr) != std::string::npos) {
                tempIndices.push_back(i);
            }
        }
        ImGui::Text("Found %zu matches. Use Next/Prev to cycle through them.", tempIndices.size());
    }
    ImGui::Separator();

    // Initial table load
    static bool initialLoadAttempted = false;
    if (!initialLoadAttempted && jsonLoader_.isLoaded() && !jsonLoader_.getIndex().empty() &&
        (currentTable_.id.empty() || currentTable_.id != jsonLoader_.getIndex()[currentIndex_].id)) {
        LOG_DEBUG("Triggering initial load for index: " + std::to_string(currentIndex_));
        if (!isTableLoading_ && !tableLoadThread_.joinable()) {
            startTableLoad(currentIndex_, settings_.vpsdbImageCacheDir);
            initialLoadAttempted = true;
        }
    }

    // Render main content
    ImGui::Columns(2, "Layout", true);
    ImGui::SetColumnWidth(0, panelWidth * 0.7f);
    ImGui::BeginChild("Metadata", ImVec2(0, -ImGui::GetFrameHeightWithSpacing() - 15.0f), false);

    ImGui::Columns(2, "Fields", false);
    float keyWidth = ImGui::CalcTextSize("tableAuthorWebsite").x + ImGui::GetStyle().FramePadding.x * 2;
    ImGui::SetColumnWidth(0, keyWidth);

    renderField("ID", currentTable_.id);
    renderField("Name", currentTable_.name);
    renderField("Manufacturer", currentTable_.manufacturer);
    renderField("Year", std::to_string(currentTable_.year));
    renderField("Theme", join(currentTable_.theme, ", "));
    renderField("Type", currentTable_.type);
    renderField("Players", std::to_string(currentTable_.players));
    renderField("Designers", join(currentTable_.designers, ", "));
    renderField("IPDB URL", currentTable_.ipdbUrl.empty() ? "Not Available" : currentTable_.ipdbUrl);
    renderField("Updated At", std::to_string(currentTable_.updatedAt));
    renderField("Last Created At", std::to_string(currentTable_.lastCreatedAt));

    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "TABLE FILES");
    ImGui::Separator();

    ImGui::PushID("TableFilesSection");
    for (size_t i = 0; i < currentTable_.tableFiles.size(); ++i) {
        const auto& file = currentTable_.tableFiles[i];
        ImGui::PushID(static_cast<int>(i));

        ImGui::Text("Table File %zu", i + 1);
        ImGui::NextColumn();

        ImGui::Text("ID: %s", file.id.c_str());
        ImGui::Text("Authors: %s", join(file.authors, ", ").c_str());
        ImGui::Text("Version: %s", file.version.c_str());
        ImGui::Text("Comment: %s", file.comment.c_str());
        ImGui::Text("Format: %s", file.tableFormat.c_str());
        ImGui::Text("Features: %s", join(file.features, ", ").c_str());

        for (size_t j = 0; j < file.urls.size(); ++j) {
            const auto& url = file.urls[j].url;
            ImGui::PushID(static_cast<int>(j));

            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.5f, 1.0f, 1.0f));
            if (ImGui::Button("Download", ImVec2(100, 0))) {
                openUrl(url);
                LOG_DEBUG("Opened URL: " + url);
            }
            if (ImGui::IsItemHovered()) {
                ImGui::BeginTooltip();
                ImGui::Text("Open link in your default browser: %s", url.c_str());
                ImGui::EndTooltip();
                ImDrawList* draw_list = ImGui::GetWindowDrawList();
                ImVec2 p1 = ImGui::GetItemRectMin();
                ImVec2 p2 = ImGui::GetItemRectMax();
                draw_list->AddLine(ImVec2(p1.x, p2.y), p2, ImGui::GetColorU32(ImVec4(0.0f, 0.5f, 1.0f, 1.0f)));
            }
            ImGui::SameLine();
            ImGui::Text("Broken: %s", file.urls[j].broken ? "Yes" : "No");
            ImGui::PopStyleColor();
            ImGui::PopID();
        }
        ImGui::NextColumn();
        ImGui::PopID();
    }
    ImGui::PopID();

    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "B2S FILES");
    ImGui::Separator();
    ImGui::PushID("B2SFilesSection");
    for (size_t i = 0; i < currentTable_.b2sFiles.size(); ++i) {
        const auto& file = currentTable_.b2sFiles[i];
        ImGui::PushID(static_cast<int>(i));

        ImGui::Text("B2S File %zu", i + 1);
        ImGui::NextColumn();
        ImGui::Text("ID: %s", file.id.c_str());
        ImGui::Text("Authors: %s", join(file.authors, ", ").c_str());
        ImGui::Text("Version: %s", file.version.c_str());
        ImGui::Text("Comment: %s", file.comment.c_str());
        ImGui::Text("Features: %s", join(file.features, ", ").c_str());
        for (size_t j = 0; j < file.urls.size(); ++j) {
            const auto& url = file.urls[j].url;
            ImGui::PushID(static_cast<int>(j));
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.5f, 1.0f, 1.0f));
            if (ImGui::Button("Download", ImVec2(100, 0))) {
                openUrl(url);
                LOG_DEBUG("Opened URL: " + url);
            }
            if (ImGui::IsItemHovered()) {
                ImGui::BeginTooltip();
                ImGui::Text("Click to open: %s", url.c_str());
                ImGui::EndTooltip();
                ImDrawList* draw_list = ImGui::GetWindowDrawList();
                ImVec2 p1 = ImGui::GetItemRectMin();
                ImVec2 p2 = ImGui::GetItemRectMax();
                draw_list->AddLine(ImVec2(p1.x, p2.y), p2, ImGui::GetColorU32(ImVec4(0.0f, 0.5f, 1.0f, 1.0f)));
            }
            ImGui::SameLine();
            ImGui::Text("Broken: %s", file.urls[j].broken ? "Yes" : "No");
            ImGui::PopStyleColor();
            ImGui::PopID();
        }
        ImGui::NextColumn();
        ImGui::PopID();
    }
    ImGui::PopID();

    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "WHEEL ART FILES");
    ImGui::Separator();
    ImGui::PushID("WheelArtFilesSection");
    for (size_t i = 0; i < currentTable_.wheelArtFiles.size(); ++i) {
        const auto& file = currentTable_.wheelArtFiles[i];
        ImGui::PushID(static_cast<int>(i));

        ImGui::Text("Wheel Art File %zu", i + 1);
        ImGui::NextColumn();
        ImGui::Text("ID: %s", file.id.c_str());
        ImGui::Text("Authors: %s", join(file.authors, ", ").c_str());
        ImGui::Text("Version: %s", file.version.c_str());
        for (size_t j = 0; j < file.urls.size(); ++j) {
            const auto& url = file.urls[j].url;
            ImGui::PushID(static_cast<int>(j));
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.5f, 1.0f, 1.0f));
            if (ImGui::Button("Download", ImVec2(100, 0))) {
                openUrl(url);
                LOG_DEBUG("Opened URL: " + url);
            }
            if (ImGui::IsItemHovered()) {
                ImGui::BeginTooltip();
                ImGui::Text("Click to open: %s", url.c_str());
                ImGui::EndTooltip();
                ImDrawList* draw_list = ImGui::GetWindowDrawList();
                ImVec2 p1 = ImGui::GetItemRectMin();
                ImVec2 p2 = ImGui::GetItemRectMax();
                draw_list->AddLine(ImVec2(p1.x, p2.y), p2, ImGui::GetColorU32(ImVec4(0.0f, 0.5f, 1.0f, 1.0f)));
            }
            ImGui::SameLine();
            ImGui::Text("Broken: %s", file.urls[j].broken ? "Yes" : "No");
            ImGui::PopStyleColor();
            ImGui::PopID();
        }
        ImGui::NextColumn();
        ImGui::PopID();
    }
    ImGui::PopID();

    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "TOPPER FILES");
    ImGui::Separator();
    ImGui::PushID("TopperFilesSection");
    for (size_t i = 0; i < currentTable_.topperFiles.size(); ++i) {
        const auto& file = currentTable_.topperFiles[i];
        ImGui::PushID(static_cast<int>(i));

        ImGui::Text("Topper File %zu", i + 1);
        ImGui::NextColumn();
        ImGui::Text("ID: %s", file.id.c_str());
        ImGui::Text("Authors: %s", join(file.authors, ", ").c_str());
        ImGui::Text("Version: %s", file.version.c_str());
        for (size_t j = 0; j < file.urls.size(); ++j) {
            const auto& url = file.urls[j].url;
            ImGui::PushID(static_cast<int>(j));
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.5f, 1.0f, 1.0f));
            if (ImGui::Button("Download", ImVec2(100, 0))) {
                openUrl(url);
                LOG_DEBUG("Opened URL: " + url);
            }
            if (ImGui::IsItemHovered()) {
                ImGui::BeginTooltip();
                ImGui::Text("Click to open: %s", url.c_str());
                ImGui::EndTooltip();
                ImDrawList* draw_list = ImGui::GetWindowDrawList();
                ImVec2 p1 = ImGui::GetItemRectMin();
                ImVec2 p2 = ImGui::GetItemRectMax();
                draw_list->AddLine(ImVec2(p1.x, p2.y), p2, ImGui::GetColorU32(ImVec4(0.0f, 0.5f, 1.0f, 1.0f)));
            }
            ImGui::SameLine();
            ImGui::Text("Broken: %s", file.urls[j].broken ? "Yes" : "No");
            ImGui::PopStyleColor();
            ImGui::PopID();
        }
        ImGui::NextColumn();
        ImGui::PopID();
    }
    ImGui::PopID();

    ImGui::Columns(1);
    ImGui::EndChild();

    ImGui::NextColumn();
    ImGui::BeginChild("Thumbnails", ImVec2(0, -ImGui::GetFrameHeightWithSpacing() - 15.0f), false);
    if (backglassTexture_) {
        ImGui::Text("Backglass");
        int backglassWidth, backglassHeight;
        SDL_QueryTexture(backglassTexture_.get(), nullptr, nullptr, &backglassWidth, &backglassHeight);
    float backglassAspectRatio = static_cast<float>(backglassHeight) / static_cast<float>(backglassWidth);
        ImVec2 displaySize(200, 200 * backglassAspectRatio);
        ImGui::Image(reinterpret_cast<ImTextureID>(backglassTexture_.get()), displaySize);
    } else {
        ImGui::Text("Backglass: Not Available");
    }
    ImGui::Spacing();
    if (playfieldTexture_) {
        ImGui::Text("Playfield");
        int playfieldWidth, playfieldHeight;
        SDL_QueryTexture(playfieldTexture_.get(), nullptr, nullptr, &playfieldWidth, &playfieldHeight);
    float aspectRatio = static_cast<float>(playfieldHeight) / static_cast<float>(playfieldWidth);
        ImGui::Image(reinterpret_cast<ImTextureID>(playfieldTexture_.get()), ImVec2(200, 200 * aspectRatio));
    } else {
        ImGui::Text("Playfield: Not Available");
    }
    ImGui::EndChild();

    ImGui::Columns(1);

    // Navigation buttons
    ImGui::SetCursorPosY(ImGui::GetWindowHeight() - ImGui::GetFrameHeightWithSpacing() - 15.0f);
    std::vector<size_t> filteredIndices;
    if (strlen(searchBuffer) > 0) {
        std::string searchStr = searchBuffer;
        std::transform(searchStr.begin(), searchStr.end(), searchStr.begin(), ::tolower);
        for (size_t i = 0; i < jsonLoader_.getIndex().size(); ++i) {
            std::string name = jsonLoader_.getIndex()[i].name;
            std::transform(name.begin(), name.end(), name.begin(), ::tolower);
            if (name.find(searchStr) != std::string::npos) {
                filteredIndices.push_back(i);
            }
        }
    }

    if (ImGui::Button("< Prev", ImVec2(100, 0))) {
        if (!isTableLoading_) {
            if (tableLoadThread_.joinable()) {
                tableLoadThread_.join();
            }
            size_t newIndex;
            if (!filteredIndices.empty()) {
                auto it = std::find(filteredIndices.begin(), filteredIndices.end(), currentIndex_);
                if (it != filteredIndices.begin()) {
                    --it;
                    newIndex = *it;
                } else {
                    newIndex = filteredIndices.back();
                }
            } else if (currentIndex_ > 0) {
                newIndex = currentIndex_ - 1;
            } else {
                newIndex = jsonLoader_.getIndex().size() - 1;
            }
            startTableLoad(newIndex, settings_.vpsdbImageCacheDir);
            LOG_DEBUG("Navigated to previous table, index: " + std::to_string(newIndex));
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Next >", ImVec2(100, 0))) {
        if (!isTableLoading_) {
            if (tableLoadThread_.joinable()) {
                tableLoadThread_.join();
            }
            size_t newIndex;
            if (!filteredIndices.empty()) {
                auto it = std::find(filteredIndices.begin(), filteredIndices.end(), currentIndex_);
                if (it != filteredIndices.end() - 1) {
                    ++it;
                    newIndex = *it;
                } else {
                    newIndex = filteredIndices.front();
                }
            } else if (currentIndex_ < jsonLoader_.getIndex().size() - 1) {
                newIndex = currentIndex_ + 1;
            } else {
                newIndex = 0;
            }
            startTableLoad(newIndex, settings_.vpsdbImageCacheDir);
            LOG_DEBUG("Navigated to next table, index: " + std::to_string(newIndex));
        }
    }

    ImGui::SameLine();
    // Align to the far right
    float closeButtonWidth = 100.0f;
    ImGui::SameLine(ImGui::GetWindowWidth() - closeButtonWidth - ImGui::GetStyle().WindowPadding.x);
    // Red close button style
    ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.7f, 0.15f, 0.15f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.85f, 0.25f, 0.25f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.6f, 0.1f, 0.1f, 1.0f));

    if (ImGui::Button("Close", ImVec2(100, 0))) {
        LOG_DEBUG("VpsdbCatalog: Close button clicked.");
        ImGui::PopStyleColor(3);
        ImGui::End(); // Manually end the window
        return false; // This signals App/Editor to close the panel
    }
    ImGui::PopStyleColor(3);

    // Loading overlay
    if (isTableLoading_) {
        ImGui::SetNextWindowPos(ImVec2(posX, posY), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(panelWidth, panelHeight), ImGuiCond_Always);
        ImGui::SetNextWindowBgAlpha(0.7f);
        ImGui::Begin("Loading Overlay", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoInputs);
        ImVec2 textSize = ImGui::CalcTextSize("Loading Table...");
        ImGui::SetCursorPos(ImVec2(panelWidth * 0.5f - textSize.x * 0.5f, panelHeight * 0.5f - textSize.y * 0.5f));
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Loading Table...");
        ImGui::End();
    }

    ImGui::End();
    return true;
}

void VpsdbCatalog::renderField(const char* key, const std::string& value) {
    ImGui::Text("%s", key);
    ImGui::NextColumn();
    ImGui::Text("%s", value.c_str());
    ImGui::NextColumn();
}

std::string VpsdbCatalog::join(const std::vector<std::string>& vec, const std::string& delim) {
    std::string result;
    for (size_t i = 0; i < vec.size(); ++i) {
        result += vec[i];
        if (i < vec.size() - 1) result += delim;
    }
    return result.empty() ? "None" : result;
}

void VpsdbCatalog::applySearchFilter(const char* searchTerm) {
    if (strlen(searchTerm) == 0) {
        return;
    }

    std::string searchStr = searchTerm;
    std::transform(searchStr.begin(), searchStr.end(), searchStr.begin(), ::tolower);

    size_t newIndex = currentIndex_;
    for (size_t i = 0; i < jsonLoader_.getIndex().size(); ++i) {
        std::string name = jsonLoader_.getIndex()[i].name;
        std::transform(name.begin(), name.end(), name.begin(), ::tolower);
        if (name.find(searchStr) != std::string::npos) {
            newIndex = i;
            break;
        }
    }
    if (newIndex != currentIndex_) {
        if (tableLoadThread_.joinable()) {
            tableLoadThread_.join();
        }
        isTableLoading_ = true;
        startTableLoad(newIndex, settings_.vpsdbImageCacheDir);
        LOG_DEBUG("Filtered to table at index: " + std::to_string(newIndex) + ", name: " + std::string(jsonLoader_.getIndex()[newIndex].name));
    }
}

void VpsdbCatalog::openUrl(const std::string& url) {
    if (url.empty()) {
        LOG_ERROR("Attempted to open empty URL");
        return;
    }
    std::string command = "xdg-open \"" + url + "\" &";
    int result = system(command.c_str());
    if (result != 0) {
        LOG_ERROR("Failed to open URL: " + url + ", system returned: " + std::to_string(result));
    }
}

} // namespace vpsdb
