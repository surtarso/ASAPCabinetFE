#include "vpsdb_catalog.h"
#include "log/logging.h"
#include <nlohmann/json.hpp>
#include <filesystem>
#include <fstream>
#include <thread>
#include <cstdlib> // For system()

namespace fs = std::filesystem;

namespace vpsdb {

VpsdbCatalog::VpsdbCatalog(const std::string& vpsdbFilePath, SDL_Renderer* renderer, const Settings& settings)
    : vpsdbFilePath_(vpsdbFilePath),
      renderer_(renderer),
      currentIndex_(0),
      loaded_(false),
      isLoading_(true),
      isTableLoading_(false),
      isOpen(false),
      backglassTexture_(nullptr, SDL_DestroyTexture),
      playfieldTexture_(nullptr, SDL_DestroyTexture),
      vpsDbClient_(std::make_unique<VpsDatabaseClient>(vpsdbFilePath_)),
      settings_(settings) {
    initThread_ = std::thread(&VpsdbCatalog::initInBackground, this);
}

VpsdbCatalog::~VpsdbCatalog() {
    if (initThread_.joinable()) {
        initThread_.join();
    }
    if (tableLoadThread_.joinable()) {
        tableLoadThread_.join();
    }
    clearThumbnails();
}

void VpsdbCatalog::initInBackground() {
    isLoading_ = true;
    progressStage_ = 1;
    LOG_DEBUG("VpsdbCatalog: Starting initialization in background");

    if (!fs::exists(vpsdbFilePath_)) {
        LOG_DEBUG("VpsdbCatalog: vpsdb.json not found, initiating fetch");
        if (!vpsDbClient_->fetchIfNeeded(settings_.vpsDbLastUpdated, settings_.vpsDbUpdateFrequency, nullptr)) {
            LOG_ERROR("VpsdbCatalog: Failed to fetch vpsdb.json");
            isLoading_ = false;
            progressStage_ = 0;
            return;
        }
    } else {
        LOG_DEBUG("VpsdbCatalog: vpsdb.json exists, checking for updates");
        if (!vpsDbClient_->fetchIfNeeded(settings_.vpsDbLastUpdated, settings_.vpsDbUpdateFrequency, nullptr)) {
            LOG_DEBUG("VpsdbCatalog: vpsdb.json exists but update check failed, proceeding with current file");
        }
    }
    progressStage_ = 2;
    loadJson(); // Ensure JSON is loaded before setting isLoading_ to false
    progressStage_ = 3;
    isLoading_ = false; // Set to false only after loadJson() completes
    LOG_DEBUG("VpsdbCatalog: Initialization complete in background");
}

void VpsdbCatalog::loadTableInBackground(size_t index) {
    LoadedTableData data;
    data.index = index;

    try {
        std::ifstream file(vpsdbFilePath_);
        nlohmann::json json;
        file >> json;
        auto entry = json[index];
        PinballTable table;
        table.id = entry.value("id", "");
        table.updatedAt = entry.value("updatedAt", 0);
        table.manufacturer = entry.value("manufacturer", "");
        table.name = entry.value("name", "");
        table.year = entry.value("year", 0);
        table.theme = entry.value("theme", std::vector<std::string>{});
        table.designers = entry.value("designers", std::vector<std::string>{});
        table.type = entry.value("type", "");
        table.players = entry.value("players", 0);
        table.ipdbUrl = entry.value("ipdbUrl", "");
        table.lastCreatedAt = entry.value("lastCreatedAt", 0);

        for (const auto& file : entry.value("tableFiles", nlohmann::json::array())) {
            TableFile tf;
            tf.id = file.value("id", "");
            tf.createdAt = file.value("createdAt", 0);
            tf.updatedAt = file.value("updatedAt", 0);
            tf.authors = file.value("authors", std::vector<std::string>{});
            tf.features = file.value("features", std::vector<std::string>{});
            tf.tableFormat = file.value("tableFormat", "");
            tf.comment = file.value("comment", "");
            tf.version = file.value("version", "");
            tf.imgUrl = file.value("imgUrl", "");
            for (const auto& url : file.value("urls", nlohmann::json::array())) {
                tf.urls.push_back({url.value("url", ""), url.value("broken", false)});
            }
            table.tableFiles.push_back(tf);
        }

        for (const auto& file : entry.value("b2sFiles", nlohmann::json::array())) {
            TableFile tf;
            tf.id = file.value("id", "");
            tf.createdAt = file.value("createdAt", 0);
            tf.updatedAt = file.value("updatedAt", 0);
            tf.authors = file.value("authors", std::vector<std::string>{});
            tf.features = file.value("features", std::vector<std::string>{});
            tf.comment = file.value("comment", "");
            tf.version = file.value("version", "");
            tf.imgUrl = file.value("imgUrl", "");
            for (const auto& url : file.value("urls", nlohmann::json::array())) {
                tf.urls.push_back({url.value("url", ""), url.value("broken", false)});
            }
            table.b2sFiles.push_back(tf);
        }

        for (const auto& file : entry.value("wheelArtFiles", nlohmann::json::array())) {
            TableFile tf;
            tf.id = file.value("id", "");
            tf.createdAt = file.value("createdAt", 0);
            tf.updatedAt = file.value("updatedAt", 0);
            tf.authors = file.value("authors", std::vector<std::string>{});
            tf.version = file.value("version", "");
            for (const auto& url : file.value("urls", nlohmann::json::array())) {
                tf.urls.push_back({url.value("url", ""), url.value("broken", false)});
            }
            table.wheelArtFiles.push_back(tf);
        }

        for (const auto& file : entry.value("topperFiles", nlohmann::json::array())) {
            TopperFile tf;
            tf.id = file.value("id", "");
            tf.createdAt = file.value("createdAt", 0);
            tf.updatedAt = file.value("updatedAt", 0);
            tf.authors = file.value("authors", std::vector<std::string>{});
            tf.version = file.value("version", "");
            for (const auto& url : file.value("urls", nlohmann::json::array())) {
                tf.urls.push_back({url.value("url", ""), url.value("broken", false)});
            }
            table.topperFiles.push_back(tf);
        }
        data.table = std::move(table);
    } catch (const std::exception& e) {
        LOG_ERROR("VpsdbCatalog: Failed to load table at index " << index << ": " << e.what());
        std::lock_guard<std::mutex> lock(mutex_);
        isTableLoading_ = false;
        return;
    }

    std::string backglassUrl, playfieldUrl;
    if (!data.table.b2sFiles.empty()) {
        backglassUrl = data.table.b2sFiles[0].imgUrl;
    }
    if (!data.table.tableFiles.empty()) {
        playfieldUrl = data.table.tableFiles[0].imgUrl;
    }

    if (!backglassUrl.empty()) {
        data.backglassPath = "data/cache/" + data.table.id + "_backglass.webp";
        if (!downloadImage(backglassUrl, data.backglassPath)) {
            data.backglassPath.clear();
        }
    }
    if (!playfieldUrl.empty()) {
        data.playfieldPath = "data/cache/" + data.table.id + "_playfield.webp";
        if (!downloadImage(playfieldUrl, data.playfieldPath)) {
            data.playfieldPath.clear();
        }
    }

    {
        std::lock_guard<std::mutex> lock(mutex_);
        loadedTableQueue_.push(std::move(data));
        isTableLoading_ = true;
    }
    LOG_DEBUG("VpsdbCatalog: Background table load complete, index: " << index);
}

bool VpsdbCatalog::render() {
    if (isLoading_) {
        ImGuiIO& io = ImGui::GetIO();
        ImGui::SetNextWindowPos(ImVec2((io.DisplaySize.x - 300) / 2, (io.DisplaySize.y - 100) / 2), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(300, 100), ImGuiCond_Always);
        ImGui::Begin("Loading VPSDB", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar);

        const char* textToDisplay = "";
        switch (progressStage_) {
            case 1:
                textToDisplay = "Fetching VPSDB...";
                break;
            case 2:
                textToDisplay = "Loading JSON...";
                break;
            default:
                textToDisplay = "Loading VPSDB...";
                break;
        }
        ImVec2 textSize = ImGui::CalcTextSize(textToDisplay);
        ImGui::SetCursorPosX((300 - textSize.x) / 2);
        ImGui::SetCursorPosY((100 - textSize.y) / 2);
        ImGui::Text("%s", textToDisplay);
        ImGui::End();
        return true;
    }

    // Wait for initialization if still running on first render
    if (initThread_.joinable()) {
        initThread_.join(); // Ensure initialization is complete
    }
    
    if (!loaded_) {
        if (!isLoading_ && index_.empty()) {
            ImGui::Text("Error: VPSDB JSON not loaded");
            LOG_ERROR("VpsdbCatalog: JSON not loaded at " << vpsdbFilePath_);
        }
        return true;
    }

    ImGuiIO& io = ImGui::GetIO();
    float panelWidth = io.DisplaySize.x * 0.7f;
    float panelHeight = io.DisplaySize.y * 0.52f;
    float posX = (io.DisplaySize.x - panelWidth) / 2.0f;
    float posY = (io.DisplaySize.y - panelHeight) / 2.0f;

    ImGui::SetNextWindowPos(ImVec2(posX, posY), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(panelWidth, panelHeight), ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.8f);

    ImGui::Begin("VPSDB Catalog", nullptr,
                 ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);

    // Process loaded table data
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!loadedTableQueue_.empty()) {
            auto data = std::move(loadedTableQueue_.front());
            loadedTableQueue_.pop();

            currentIndex_ = data.index;
            currentTable_ = std::move(data.table);
            clearThumbnails();

            if (!data.backglassPath.empty()) {
                backglassTexture_.reset(loadTexture(data.backglassPath));
                currentBackglassPath_ = data.backglassPath;
            }
            if (!data.playfieldPath.empty()) {
                playfieldTexture_.reset(loadTexture(data.playfieldPath));
                currentPlayfieldPath_ = data.playfieldPath;
            }

            isTableLoading_ = false;
            LOG_DEBUG("VpsdbCatalog: Processed loaded table, index: " << currentIndex_);
        }
    }

    // Search input and button
    static char searchBuffer[256] = "";
    ImGui::InputText("##Search", searchBuffer, IM_ARRAYSIZE(searchBuffer), ImGuiInputTextFlags_EnterReturnsTrue); // Fixed to avoid crash
    ImGui::SameLine();
    if (ImGui::Button("Fetch") || ImGui::IsKeyPressed(ImGuiKey_Enter, true)) {
        applySearchFilter(searchBuffer);
    }
    if (strlen(searchBuffer) > 0) {
        std::string searchStr = searchBuffer;
        std::transform(searchStr.begin(), searchStr.end(), searchStr.begin(), ::tolower);
        std::vector<size_t> tempIndices;
        for (size_t i = 0; i < index_.size(); ++i) {
            std::string name = index_[i].name;
            std::transform(name.begin(), name.end(), name.begin(), ::tolower);
            if (name.find(searchStr) != std::string::npos) {
                tempIndices.push_back(i);
            }
        }
        ImGui::Text("Found %zu matches. Use Next/Prev to cycle through them.", tempIndices.size());
    }
    ImGui::Separator();

    // Initial table load
    if (currentTable_.id.empty() || currentTable_.id != index_[currentIndex_].id) {
        if (!isTableLoading_ && !tableLoadThread_.joinable()) {
            isTableLoading_ = true;
            tableLoadThread_ = std::thread(&VpsdbCatalog::loadTableInBackground, this, currentIndex_);
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
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "TABLE FILES");
    ImGui::Separator();

    // We will push unique IDs per table file within the loop itself.
    // So, remove the ImGui::PushID("TableFiles"); from here.

    for (size_t i = 0; i < currentTable_.tableFiles.size(); ++i) {
        const auto& file = currentTable_.tableFiles[i];

        // Push a unique ID for each table file entry.
        // Using the index 'i' is a common and safe way to ensure uniqueness within a loop.
        ImGui::PushID(static_cast<int>(i));

        ImGui::Text("Table File %zu", i + 1);
        ImGui::NextColumn(); // This might need to be adjusted based on your column setup, ensure it's correct for your layout.

        ImGui::Text("ID: %s", file.id.c_str());
        ImGui::Text("Authors: %s", join(file.authors, ", ").c_str());
        ImGui::Text("Version: %s", file.version.c_str());
        ImGui::Text("Comment: %s", file.comment.c_str());
        ImGui::Text("Format: %s", file.tableFormat.c_str());
        ImGui::Text("Features: %s", join(file.features, ", ").c_str());

        for (size_t j = 0; j < file.urls.size(); ++j) {
            const auto& url = file.urls[j].url;
            // You already have ImGui::PushID(j) here, which is correct for the inner loop.
            // This ensures each "Download" button for a URL within the current table file is unique.
            ImGui::PushID(static_cast<int>(j)); // Scope ID for each URL within this table file

            // The snprintf for buttonId is technically redundant when PushID is used,
            // as ImGui's ID stack handles the uniqueness. You can remove this line if desired.
            // char buttonId[54];
            // snprintf(buttonId, sizeof(buttonId), "table_url_%zu_%zu", i, j);

            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.5f, 1.0f, 1.0f));
            if (ImGui::Button("Download", ImVec2(100, 0))) {
                openUrl(url);
                LOG_DEBUG("VpsdbCatalog: Opened URL: " << url);
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
            ImGui::PopID(); // End scope for each URL
        }
        ImGui::NextColumn(); // Again, verify column usage
        ImGui::PopID(); // Pop the ID for the current table file, before the next iteration.
    }

    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "B2S FILES");
    ImGui::Separator();
    // Removed ImGui::PushID("B2SFiles"); - IDs will be pushed per-item in the loop
    for (size_t i = 0; i < currentTable_.b2sFiles.size(); ++i) {
        const auto& file = currentTable_.b2sFiles[i];
        ImGui::PushID(static_cast<int>(i)); // Push unique ID for each B2S file

        ImGui::Text("B2S File %zu", i + 1);
        ImGui::NextColumn();
        ImGui::Text("ID: %s", file.id.c_str());
        ImGui::Text("Authors: %s", join(file.authors, ", ").c_str());
        ImGui::Text("Version: %s", file.version.c_str());
        ImGui::Text("Comment: %s", file.comment.c_str());
        ImGui::Text("Features: %s", join(file.features, ", ").c_str());
        for (size_t j = 0; j < file.urls.size(); ++j) {
            const auto& url = file.urls[j].url;
            ImGui::PushID(static_cast<int>(j)); // Scope ID for each URL
            // char buttonId[54]; // Redundant with PushID
            // snprintf(buttonId, sizeof(buttonId), "b2s_url_%zu_%zu", i, j);
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.5f, 1.0f, 1.0f));
            if (ImGui::Button("Download", ImVec2(100, 0))) {
                openUrl(url);
                LOG_DEBUG("VpsdbCatalog: Opened URL: " << url);
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
            ImGui::PopID(); // End scope for each URL
        }
        ImGui::NextColumn();
        ImGui::PopID(); // Pop the ID for the current B2S file
    }
    // Removed ImGui::PopID();

    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "WHEEL ART FILES");
    ImGui::Separator();
    // Removed ImGui::PushID("WheelFiles"); - IDs will be pushed per-item in the loop
    for (size_t i = 0; i < currentTable_.wheelArtFiles.size(); ++i) {
        const auto& file = currentTable_.wheelArtFiles[i];
        ImGui::PushID(static_cast<int>(i)); // Push unique ID for each Wheel Art file

        ImGui::Text("Wheel Art File %zu", i + 1);
        ImGui::NextColumn();
        ImGui::Text("ID: %s", file.id.c_str());
        ImGui::Text("Authors: %s", join(file.authors, ", ").c_str());
        ImGui::Text("Version: %s", file.version.c_str());
        for (size_t j = 0; j < file.urls.size(); ++j) {
            const auto& url = file.urls[j].url;
            ImGui::PushID(static_cast<int>(j)); // Scope ID for each URL
            // char buttonId[54]; // Redundant with PushID
            // snprintf(buttonId, sizeof(buttonId), "wheel_url_%zu_%zu", i, j);
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.5f, 1.0f, 1.0f));
            if (ImGui::Button("Download", ImVec2(100, 0))) {
                openUrl(url);
                LOG_DEBUG("VpsdbCatalog: Opened URL: " << url);
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
            ImGui::PopID(); // End scope for each URL
        }
        ImGui::NextColumn();
        ImGui::PopID(); // Pop the ID for the current Wheel Art file
    }
    // Removed ImGui::PopID();

    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "TOPPER FILES");
    ImGui::Separator();
    // Removed ImGui::PushID("TopperFiles"); - IDs will be pushed per-item in the loop
    for (size_t i = 0; i < currentTable_.topperFiles.size(); ++i) {
        const auto& file = currentTable_.topperFiles[i];
        ImGui::PushID(static_cast<int>(i)); // Push unique ID for each Topper file

        ImGui::Text("Topper File %zu", i + 1);
        ImGui::NextColumn();
        ImGui::Text("ID: %s", file.id.c_str());
        ImGui::Text("Authors: %s", join(file.authors, ", ").c_str());
        ImGui::Text("Version: %s", file.version.c_str());
        for (size_t j = 0; j < file.urls.size(); ++j) {
            const auto& url = file.urls[j].url;
            ImGui::PushID(static_cast<int>(j)); // Scope ID for each URL
            // char buttonId[54]; // Redundant with PushID
            // snprintf(buttonId, sizeof(buttonId), "topper_url_%zu_%zu", i, j);
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.5f, 1.0f, 1.0f));
            if (ImGui::Button("Download", ImVec2(100, 0))) {
                openUrl(url);
                LOG_DEBUG("VpsdbCatalog: Opened URL: " << url);
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
            ImGui::PopID(); // End scope for each URL
        }
        ImGui::NextColumn();
        ImGui::PopID(); // Pop the ID for the current Topper file
    }
    // Removed ImGui::PopID();

    ImGui::Columns(1);
    ImGui::EndChild();

    ImGui::NextColumn();
    ImGui::BeginChild("Thumbnails", ImVec2(0, -ImGui::GetFrameHeightWithSpacing() - 15.0f), false);
    if (backglassTexture_) {
        ImGui::Text("Backglass");
        int backglassWidth, backglassHeight;
        SDL_QueryTexture(backglassTexture_.get(), nullptr, nullptr, &backglassWidth, &backglassHeight);
        float backglassAspectRatio = static_cast<float>(backglassHeight) / backglassWidth;
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
        float aspectRatio = static_cast<float>(playfieldHeight) / playfieldWidth;
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
        for (size_t i = 0; i < index_.size(); ++i) {
            std::string name = index_[i].name;
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
                newIndex = index_.size() - 1;
            }
            isTableLoading_ = true;
            tableLoadThread_ = std::thread(&VpsdbCatalog::loadTableInBackground, this, newIndex);
            LOG_DEBUG("VpsdbCatalog: Navigated to previous table, index: " << newIndex);
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
            } else if (currentIndex_ < index_.size() - 1) {
                newIndex = currentIndex_ + 1;
            } else {
                newIndex = 0;
            }
            isTableLoading_ = true;
            tableLoadThread_ = std::thread(&VpsdbCatalog::loadTableInBackground, this, newIndex);
            LOG_DEBUG("VpsdbCatalog: Navigated to next table, index: " << newIndex);
        }
    }

    // Loading overlay
    if (isTableLoading_) {
        ImGui::SetNextWindowPos(ImVec2(posX, posY), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(panelWidth, panelHeight), ImGuiCond_Always);
        ImGui::SetNextWindowBgAlpha(0.7f); // Added for transparency
        ImGui::Begin("Loading Overlay", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoInputs);
        ImVec2 textSize = ImGui::CalcTextSize("Loading Table...");
        ImGui::SetCursorPos(ImVec2(panelWidth * 0.5f - textSize.x * 0.5f, panelHeight * 0.5f - textSize.y * 0.5f));
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Loading Table..."); // Yellow text
        ImGui::End();
    }

    ImGui::End();
    return true;
}

void VpsdbCatalog::loadJson() {
    try {
        std::ifstream file(vpsdbFilePath_);
        if (!file.is_open()) {
            LOG_ERROR("VpsdbCatalog: Failed to open JSON file: " << vpsdbFilePath_);
            loaded_ = false;
            return;
        }
        nlohmann::json json;
        file >> json;
        index_.clear();
        for (const auto& entry : json) {
            TableIndex idx;
            idx.id = entry.value("id", "");
            idx.name = entry.value("name", "");
            idx.manufacturer = entry.value("manufacturer", "");
            idx.year = entry.value("year", 0);
            index_.push_back(idx);
        }
        loaded_ = true;
        LOG_INFO("VpsdbCatalog: Loaded " << index_.size() << " tables from JSON");
    } catch (const std::exception& e) {
        LOG_ERROR("VpsdbCatalog: JSON parsing error: " << e.what());
        loaded_ = false;
    }
}

void VpsdbCatalog::loadTable(size_t index) {
    try {
        std::ifstream file(vpsdbFilePath_);
        nlohmann::json json;
        file >> json;
        auto entry = json[index];
        currentTable_ = {};
        currentTable_.id = entry.value("id", "");
        currentTable_.updatedAt = entry.value("updatedAt", 0);
        currentTable_.manufacturer = entry.value("manufacturer", "");
        currentTable_.name = entry.value("name", "");
        currentTable_.year = entry.value("year", 0);
        currentTable_.theme = entry.value("theme", std::vector<std::string>{});
        currentTable_.designers = entry.value("designers", std::vector<std::string>{});
        currentTable_.type = entry.value("type", "");
        currentTable_.players = entry.value("players", 0);
        currentTable_.ipdbUrl = entry.value("ipdbUrl", "");
        currentTable_.lastCreatedAt = entry.value("lastCreatedAt", 0);

        for (const auto& file : entry.value("tableFiles", nlohmann::json::array())) {
            TableFile tf;
            tf.id = file.value("id", "");
            tf.createdAt = file.value("createdAt", 0);
            tf.updatedAt = file.value("updatedAt", 0);
            tf.authors = file.value("authors", std::vector<std::string>{});
            tf.features = file.value("features", std::vector<std::string>{});
            tf.tableFormat = file.value("tableFormat", "");
            tf.comment = file.value("comment", "");
            tf.version = file.value("version", "");
            tf.imgUrl = file.value("imgUrl", "");
            for (const auto& url : file.value("urls", nlohmann::json::array())) {
                tf.urls.push_back({url.value("url", ""), url.value("broken", false)});
            }
            currentTable_.tableFiles.push_back(tf);
        }

        for (const auto& file : entry.value("b2sFiles", nlohmann::json::array())) {
            TableFile tf;
            tf.id = file.value("id", "");
            tf.createdAt = file.value("createdAt", 0);
            tf.updatedAt = file.value("updatedAt", 0);
            tf.authors = file.value("authors", std::vector<std::string>{});
            tf.features = file.value("features", std::vector<std::string>{});
            tf.comment = file.value("comment", "");
            tf.version = file.value("version", "");
            tf.imgUrl = file.value("imgUrl", "");
            for (const auto& url : file.value("urls", nlohmann::json::array())) {
                tf.urls.push_back({url.value("url", ""), url.value("broken", false)});
            }
            currentTable_.b2sFiles.push_back(tf);
        }

        for (const auto& file : entry.value("wheelArtFiles", nlohmann::json::array())) {
            TableFile tf;
            tf.id = file.value("id", "");
            tf.createdAt = file.value("createdAt", 0);
            tf.updatedAt = file.value("updatedAt", 0);
            tf.authors = file.value("authors", std::vector<std::string>{});
            tf.version = file.value("version", "");
            for (const auto& url : file.value("urls", nlohmann::json::array())) {
                tf.urls.push_back({url.value("url", ""), url.value("broken", false)});
            }
            currentTable_.wheelArtFiles.push_back(tf);
        }

        for (const auto& file : entry.value("topperFiles", nlohmann::json::array())) {
            TopperFile tf;
            tf.id = file.value("id", "");
            tf.createdAt = file.value("createdAt", 0);
            tf.updatedAt = file.value("updatedAt", 0);
            tf.authors = file.value("authors", std::vector<std::string>{});
            tf.version = file.value("version", "");
            for (const auto& url : file.value("urls", nlohmann::json::array())) {
                tf.urls.push_back({url.value("url", ""), url.value("broken", false)});
            }
            currentTable_.topperFiles.push_back(tf);
        }

        LOG_DEBUG("VpsdbCatalog: Loaded table at index: " << index << ", name: " << currentTable_.name);
    } catch (const std::exception& e) {
        LOG_ERROR("VpsdbCatalog: Failed to load table at index " << index << ": " << e.what());
        currentTable_ = {};
    }
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
    for (size_t i = 0; i < index_.size(); ++i) {
        std::string name = index_[i].name;
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
        tableLoadThread_ = std::thread(&VpsdbCatalog::loadTableInBackground, this, newIndex);
        LOG_DEBUG("VpsdbCatalog: Filtered to table at index: " << newIndex << ", name: " << index_[newIndex].name);
    }
}

void VpsdbCatalog::openUrl(const std::string& url) {
    if (url.empty()) {
        LOG_ERROR("VpsdbCatalog: Attempted to open empty URL");
        return;
    }
    // On Linux, use xdg-open to launch the default browser
    std::string command = "xdg-open \"" + url + "\" &";
    int result = system(command.c_str());
    if (result != 0) {
        LOG_ERROR("VpsdbCatalog: Failed to open URL: " << url << ", system returned: " << result);
    }
}

} // namespace vpsdb