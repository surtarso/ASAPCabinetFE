#include "vpsdb_catalog.h"
#include "log/logging.h"
#include <nlohmann/json.hpp>
#include <filesystem>
#include <fstream>
#include <thread> // For std::this_thread

namespace fs = std::filesystem;

namespace vpsdb {

VpsdbCatalog::VpsdbCatalog(const std::string& vpsdbFilePath, SDL_Renderer* renderer, const Settings& settings)
    : vpsdbFilePath_(vpsdbFilePath),
      renderer_(renderer),
      currentIndex_(0),
      loaded_(false),
      isLoading_(false),
      backglassTexture_(nullptr, SDL_DestroyTexture),
      playfieldTexture_(nullptr, SDL_DestroyTexture),
      vpsDbClient_(std::make_unique<VpsDatabaseClient>(vpsdbFilePath_)),
      settings_(settings) {
    initThread_ = std::thread(&VpsdbCatalog::initInBackground, this);
}

VpsdbCatalog::~VpsdbCatalog() {
    if (initThread_.joinable()) {
        initThread_.join(); // Wait for the thread to finish
    }
    clearThumbnails();
}

void VpsdbCatalog::initInBackground() {
    isLoading_ = true;
    progressStage_ = 1; // Start with fetching stage
    LOG_DEBUG("VpsdbCatalog: Starting initialization in background");

    // Check and fetch vpsdb.json if needed
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
    progressStage_ = 2; // Move to loading JSON stage

    // Load the JSON after fetch
    loadJson();
    progressStage_ = 3; // Done
    isLoading_ = false;
    LOG_DEBUG("VpsdbCatalog: Initialization complete in background");
}

bool VpsdbCatalog::render() {
    if (isLoading_) {
        ImGuiIO& io = ImGui::GetIO();
        ImGui::SetNextWindowPos(ImVec2((io.DisplaySize.x - 300) / 2, (io.DisplaySize.y - 100) / 2), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(300, 100), ImGuiCond_Always);
        ImGui::Begin("Loading VPSDB", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar);

        // Calculate text size for centering
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

        // Center text horizontally
        ImGui::SetCursorPosX((300 - textSize.x) / 2);
        // Center text vertically
        ImGui::SetCursorPosY((100 - textSize.y) / 2);

        ImGui::Text("%s", textToDisplay); // Use %s to display the dynamic text

        ImGui::End();
        return true;
    }
    
    if (!loaded_) {
        ImGui::Text("Error: VPSDB JSON not loaded");
        LOG_ERROR("VpsdbCatalog: JSON not loaded at " << vpsdbFilePath_);
        return true;
    }

    // Center and fix window size (matching TableOverrideEditor style)
    ImGuiIO& io = ImGui::GetIO();
    float panelWidth = io.DisplaySize.x * 0.7f;  // 70% of screen width
    float panelHeight = io.DisplaySize.y * 0.52f; // 52% of screen height
    float posX = (io.DisplaySize.x - panelWidth) / 2.0f;
    float posY = (io.DisplaySize.y - panelHeight) / 2.0f;

    ImGui::SetNextWindowPos(ImVec2(posX, posY), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(panelWidth, panelHeight), ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.8f); // Semi-transparent

    ImGui::Begin("VPSDB Catalog", nullptr,
                 ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);

    // Add search bar and apply filter
    static char searchBuffer[256] = "";
    ImGui::InputText("Search", searchBuffer, IM_ARRAYSIZE(searchBuffer));
    ImGui::Separator();
    applySearchFilter(searchBuffer);

    // Load current table data if needed
    if (currentTable_.id.empty() || currentTable_.id != index_[currentIndex_].id) {
        loadTable(currentIndex_);
        clearThumbnails();
        loadThumbnails();
    }

    // Split panel into left (metadata) and right (thumbnails)
    ImGui::Columns(2, "Layout", true);
    ImGui::SetColumnWidth(0, panelWidth * 0.7f); // 70% for metadata
    ImGui::BeginChild("Metadata", ImVec2(0, -ImGui::GetFrameHeightWithSpacing() - 15.0f), false);

    // Render metadata in two columns
    ImGui::Columns(2, "Fields", false);
    float keyWidth = ImGui::CalcTextSize("tableAuthorWebsite").x + ImGui::GetStyle().FramePadding.x * 2;
    ImGui::SetColumnWidth(0, keyWidth);

    // Basic fields
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

    // Table Files
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "TABLE FILES");
    ImGui::Separator();
    for (size_t i = 0; i < currentTable_.tableFiles.size(); ++i) {
        const auto& file = currentTable_.tableFiles[i];
        ImGui::Text("Table File %zu", i + 1);
        ImGui::NextColumn();
        ImGui::Text("ID: %s", file.id.c_str());
        ImGui::Text("Authors: %s", join(file.authors, ", ").c_str());
        ImGui::Text("Version: %s", file.version.c_str());
        ImGui::Text("Comment: %s", file.comment.c_str());
        ImGui::Text("Format: %s", file.tableFormat.c_str());
        ImGui::Text("Features: %s", join(file.features, ", ").c_str());
        ImGui::Text("Image URL: %s", file.imgUrl.c_str());
        for (size_t j = 0; j < file.urls.size(); ++j) {
            ImGui::Text("URL %zu: %s", j + 1, file.urls[j].url.c_str());
            ImGui::Text("Broken: %s", file.urls[j].broken ? "Yes" : "No");
        }
        ImGui::NextColumn();
    }

    // B2S Files
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "B2S FILES");
    ImGui::Separator();
    for (size_t i = 0; i < currentTable_.b2sFiles.size(); ++i) {
        const auto& file = currentTable_.b2sFiles[i];
        ImGui::Text("B2S File %zu", i + 1);
        ImGui::NextColumn();
        ImGui::Text("ID: %s", file.id.c_str());
        ImGui::Text("Authors: %s", join(file.authors, ", ").c_str());
        ImGui::Text("Version: %s", file.version.c_str());
        ImGui::Text("Comment: %s", file.comment.c_str());
        ImGui::Text("Features: %s", join(file.features, ", ").c_str());
        ImGui::Text("Image URL: %s", file.imgUrl.c_str());
        for (size_t j = 0; j < file.urls.size(); ++j) {
            ImGui::Text("URL %zu: %s", j + 1, file.urls[j].url.c_str());
            ImGui::Text("Broken: %s", file.urls[j].broken ? "Yes" : "No");
        }
        ImGui::NextColumn();
    }

    // Wheel Art Files
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "WHEEL ART FILES");
    ImGui::Separator();
    for (size_t i = 0; i < currentTable_.wheelArtFiles.size(); ++i) {
        const auto& file = currentTable_.wheelArtFiles[i];
        ImGui::Text("Wheel Art File %zu", i + 1);
        ImGui::NextColumn();
        ImGui::Text("ID: %s", file.id.c_str());
        ImGui::Text("Authors: %s", join(file.authors, ", ").c_str());
        ImGui::Text("Version: %s", file.version.c_str());
        for (size_t j = 0; j < file.urls.size(); ++j) {
            ImGui::Text("URL %zu: %s", j + 1, file.urls[j].url.c_str());
            ImGui::Text("Broken: %s", file.urls[j].broken ? "Yes" : "No");
        }
        ImGui::NextColumn();
    }

    // Topper Files
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "TOPPER FILES");
    ImGui::Separator();
    for (size_t i = 0; i < currentTable_.topperFiles.size(); ++i) {
        const auto& file = currentTable_.topperFiles[i];
        ImGui::Text("Topper File %zu", i + 1);
        ImGui::NextColumn();
        ImGui::Text("ID: %s", file.id.c_str());
        ImGui::Text("Authors: %s", join(file.authors, ", ").c_str());
        ImGui::Text("Version: %s", file.version.c_str());
        for (size_t j = 0; j < file.urls.size(); ++j) {
            ImGui::Text("URL %zu: %s", j + 1, file.urls[j].url.c_str());
            ImGui::Text("Broken: %s", file.urls[j].broken ? "Yes" : "No");
        }
        ImGui::NextColumn();
    }

    ImGui::Columns(1);
    ImGui::EndChild();

    // Render thumbnails in right column
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

    // Navigation buttons with filtered cycling
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
        if (!filteredIndices.empty()) {
            auto it = std::find(filteredIndices.begin(), filteredIndices.end(), currentIndex_);
            if (it != filteredIndices.begin()) {
                --it;
                currentIndex_ = *it;
            } else {
                currentIndex_ = filteredIndices.back();
            }
        } else if (currentIndex_ > 0) {
            currentIndex_--;
        } else {
            currentIndex_ = index_.size() - 1;
        }
        currentTable_ = {};
        clearThumbnails();
        loadTable(currentIndex_);
        loadThumbnails();
        LOG_DEBUG("VpsdbCatalog: Navigated to previous table, index: " << currentIndex_);
    }
    ImGui::SameLine();
    if (ImGui::Button("Next >", ImVec2(100, 0))) {
        if (!filteredIndices.empty()) {
            auto it = std::find(filteredIndices.begin(), filteredIndices.end(), currentIndex_);
            if (it != filteredIndices.end() - 1) {
                ++it;
                currentIndex_ = *it;
            } else {
                currentIndex_ = filteredIndices.front();
            }
        } else if (currentIndex_ < index_.size() - 1) {
            currentIndex_++;
        } else {
            currentIndex_ = 0;
        }
        currentTable_ = {};
        clearThumbnails();
        loadTable(currentIndex_);
        loadThumbnails();
        LOG_DEBUG("VpsdbCatalog: Navigated to next table, index: " << currentIndex_);
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
        return; // No filtering if search is empty
    }

    static std::string lastSearchTerm = "";
    std::string searchStr = searchTerm;
    std::transform(searchStr.begin(), searchStr.end(), searchStr.begin(), ::tolower);

    if (searchStr != lastSearchTerm) {
        size_t newIndex = currentIndex_;
        for (size_t i = 0; i < index_.size(); ++i) {
            std::string name = index_[i].name;
            std::transform(name.begin(), name.end(), name.begin(), ::tolower);
            if (name.find(searchStr) != std::string::npos) {
                newIndex = i;
                break; // Stop at first match
            }
        }
        if (newIndex != currentIndex_) {
            currentIndex_ = newIndex;
            currentTable_ = {};
            clearThumbnails();
            loadTable(currentIndex_);
            loadThumbnails();
            LOG_DEBUG("VpsdbCatalog: Filtered to table at index: " << currentIndex_ << ", name: " << index_[currentIndex_].name);
        }
        lastSearchTerm = searchStr;
    }
}

} // namespace vpsdb