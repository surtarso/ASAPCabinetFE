#include "vpsdb_catalog_table.h"
#include "vpsdb_catalog_image.h"
#include "log/logging.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <stdexcept>
#include <filesystem>

namespace fs = std::filesystem;

namespace vpsdb {

PinballTable loadTableFromJson(const std::string& vpsdbFilePath, size_t index) {
    try {
        std::ifstream file(vpsdbFilePath);
        if (!file.is_open()) {
            throw std::runtime_error("Failed to open JSON file: " + vpsdbFilePath);
        }
        nlohmann::json json;
        file >> json;
        if (index >= json.size()) {
            throw std::out_of_range("Index out of range: " + std::to_string(index));
        }
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
        return table;
    } catch (const std::exception& e) {
        LOG_ERROR("VpsdbCatalog: Failed to load table at index " << index << ": " << e.what());
        return PinballTable{};
    }
}

void loadTableInBackground(const std::string& vpsdbFilePath, size_t index,
                           std::queue<LoadedTableData>& loadedTableQueue,
                           std::mutex& mutex, std::atomic<bool>& isTableLoading,
                           const std::string& exePath) {
    LOG_DEBUG("VpsdbCatalog: Starting background load for index: " << index);
    LOG_DEBUG("VpsdbCatalog: exePath = " << exePath);
    LoadedTableData data;
    data.index = index;
    data.table = loadTableFromJson(vpsdbFilePath, index);
    if (data.table.id.empty()) {
        std::lock_guard<std::mutex> lock(mutex);
        isTableLoading = false;
        LOG_ERROR("VpsdbCatalog: Failed to load table for index: " << index << ", empty ID");
        return;
    }
    LOG_DEBUG("VpsdbCatalog: Loaded table data for index: " << index << ", name: " << data.table.name);

    std::string backglassUrl, playfieldUrl;
    if (!data.table.b2sFiles.empty()) {
        backglassUrl = data.table.b2sFiles[0].imgUrl;
        LOG_DEBUG("VpsdbCatalog: Backglass URL for index " << index << ": " << backglassUrl);
    }
    if (!data.table.tableFiles.empty()) {
        playfieldUrl = data.table.tableFiles[0].imgUrl;
        LOG_DEBUG("VpsdbCatalog: Playfield URL for index " << index << ": " << playfieldUrl);
    }

    fs::path exeDir = fs::path(exePath).parent_path(); // Get directory of executable
    fs::path cachePath = exeDir / "data/cache";
    fs::create_directories(cachePath);
    LOG_DEBUG("VpsdbCatalog: Cache dir = " << cachePath.string());

    if (!backglassUrl.empty()) {
        data.backglassPath = (cachePath / (data.table.id + "_backglass.webp")).string();
        LOG_DEBUG("VpsdbCatalog: Resolved backglassPath = " << data.backglassPath);
        if (!VpsdbImage::downloadImage(backglassUrl, data.backglassPath)) {
            data.backglassPath.clear();
            LOG_ERROR("VpsdbCatalog: Failed to download backglass for index: " << index);
        } else {
            LOG_DEBUG("VpsdbCatalog: Downloaded backglass to: " << data.backglassPath);
        }
    }
    if (!playfieldUrl.empty()) {
        data.playfieldPath = (cachePath / (data.table.id + "_playfield.webp")).string();
        LOG_DEBUG("VpsdbCatalog: Resolved playfieldPath = " << data.playfieldPath);
        if (!VpsdbImage::downloadImage(playfieldUrl, data.playfieldPath)) {
            data.playfieldPath.clear();
            LOG_ERROR("VpsdbCatalog: Failed to download playfield for index: " << index);
        } else {
            LOG_DEBUG("VpsdbCatalog: Downloaded playfield to: " << data.playfieldPath);
        }
    }

    {
        std::lock_guard<std::mutex> lock(mutex);
        loadedTableQueue.push(std::move(data));
        LOG_DEBUG("VpsdbCatalog: Enqueued table data for index: " << index);
    }
    LOG_DEBUG("VpsdbCatalog: Background table load complete, index: " << index);
}

} // namespace vpsdb