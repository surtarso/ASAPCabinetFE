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

    for (const auto& entryFile : entry.value("tableFiles", nlohmann::json::array())) {
            TableFile tf;
            tf.id = entryFile.value("id", "");
            tf.createdAt = entryFile.value("createdAt", 0);
            tf.updatedAt = entryFile.value("updatedAt", 0);
            tf.authors = entryFile.value("authors", std::vector<std::string>{});
            tf.features = entryFile.value("features", std::vector<std::string>{});
            tf.tableFormat = entryFile.value("tableFormat", "");
            tf.comment = entryFile.value("comment", "");
            tf.version = entryFile.value("version", "");
            tf.imgUrl = entryFile.value("imgUrl", "");
            for (const auto& url : entryFile.value("urls", nlohmann::json::array())) {
                tf.urls.push_back({url.value("url", ""), url.value("broken", false)});
            }
            table.tableFiles.push_back(tf);
        }

    for (const auto& entryFile : entry.value("b2sFiles", nlohmann::json::array())) {
            TableFile tf;
            tf.id = entryFile.value("id", "");
            tf.createdAt = entryFile.value("createdAt", 0);
            tf.updatedAt = entryFile.value("updatedAt", 0);
            tf.authors = entryFile.value("authors", std::vector<std::string>{});
            tf.features = entryFile.value("features", std::vector<std::string>{});
            tf.comment = entryFile.value("comment", "");
            tf.version = entryFile.value("version", "");
            tf.imgUrl = entryFile.value("imgUrl", "");
            for (const auto& url : entryFile.value("urls", nlohmann::json::array())) {
                tf.urls.push_back({url.value("url", ""), url.value("broken", false)});
            }
            table.b2sFiles.push_back(tf);
        }

    for (const auto& entryFile : entry.value("wheelArtFiles", nlohmann::json::array())) {
            TableFile tf;
            tf.id = entryFile.value("id", "");
            tf.createdAt = entryFile.value("createdAt", 0);
            tf.updatedAt = entryFile.value("updatedAt", 0);
            tf.authors = entryFile.value("authors", std::vector<std::string>{});
            tf.version = entryFile.value("version", "");
            for (const auto& url : entryFile.value("urls", nlohmann::json::array())) {
                tf.urls.push_back({url.value("url", ""), url.value("broken", false)});
            }
            table.wheelArtFiles.push_back(tf);
        }

    for (const auto& entryFile : entry.value("topperFiles", nlohmann::json::array())) {
            TopperFile tf;
            tf.id = entryFile.value("id", "");
            tf.createdAt = entryFile.value("createdAt", 0);
            tf.updatedAt = entryFile.value("updatedAt", 0);
            tf.authors = entryFile.value("authors", std::vector<std::string>{});
            tf.version = entryFile.value("version", "");
            for (const auto& url : entryFile.value("urls", nlohmann::json::array())) {
                tf.urls.push_back({url.value("url", ""), url.value("broken", false)});
            }
            table.topperFiles.push_back(tf);
        }
        return table;
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to load table at index " + std::to_string(index) + ": " + std::string(e.what()));
        return PinballTable{};
    }
}

void loadTableInBackground(const std::string& vpsdbFilePath, size_t index,
                           std::queue<LoadedTableData>& loadedTableQueue,
                           std::mutex& mutex, std::atomic<bool>& isTableLoading,
                           const std::string& exePath) {
    LOG_DEBUG("Starting background load for index: " + std::to_string(index));
    LOG_DEBUG("exePath = " + exePath);
    LoadedTableData data;
    data.index = index;
    data.table = loadTableFromJson(vpsdbFilePath, index);
    if (data.table.id.empty()) {
        std::lock_guard<std::mutex> lock(mutex);
        isTableLoading = false;
        LOG_ERROR("Failed to load table for index: " + std::to_string(index) + ", empty ID");
        return;
    }
    LOG_DEBUG("Loaded table data for index: " + std::to_string(index) + ", name: " + data.table.name);

    std::string backglassUrl, playfieldUrl;
    if (!data.table.b2sFiles.empty()) {
        backglassUrl = data.table.b2sFiles[0].imgUrl;
        LOG_DEBUG("Backglass URL for index " + std::to_string(index) + ": " + backglassUrl);
    }
    if (!data.table.tableFiles.empty()) {
        playfieldUrl = data.table.tableFiles[0].imgUrl;
        LOG_DEBUG("Playfield URL for index " + std::to_string(index) + ": " + playfieldUrl);
    }

    fs::path exeDir = fs::path(exePath).parent_path(); // Get directory of executable
    fs::path cachePath = exeDir / "data/cache";
    fs::create_directories(cachePath);
    LOG_DEBUG("Cache dir = " + cachePath.string());

    if (!backglassUrl.empty()) {
        data.backglassPath = (cachePath / (data.table.id + "_backglass.webp")).string();
        LOG_DEBUG("Resolved backglassPath = " + data.backglassPath);
        if (!VpsdbImage::downloadImage(backglassUrl, data.backglassPath)) {
            data.backglassPath.clear();
            LOG_ERROR("Failed to download backglass for index: " + std::to_string(index));
        } else {
            LOG_DEBUG("Downloaded backglass to: " + data.backglassPath);
        }
    }
    if (!playfieldUrl.empty()) {
        data.playfieldPath = (cachePath / (data.table.id + "_playfield.webp")).string();
        LOG_DEBUG("Resolved playfieldPath = " + data.playfieldPath);
        if (!VpsdbImage::downloadImage(playfieldUrl, data.playfieldPath)) {
            data.playfieldPath.clear();
            LOG_ERROR("Failed to download playfield for index: " + std::to_string(index));
        } else {
            LOG_DEBUG("Downloaded playfield to: " + data.playfieldPath);
        }
    }

    {
        std::lock_guard<std::mutex> lock(mutex);
        loadedTableQueue.push(std::move(data));
        LOG_DEBUG("Enqueued table data for index: " + std::to_string(index));
    }
    LOG_DEBUG("Background table load complete, index: " + std::to_string(index));
}

} // namespace vpsdb
