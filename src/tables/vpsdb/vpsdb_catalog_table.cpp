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

void loadTableInBackground(const nlohmann::json vpsdbJson,
                           size_t index,
                           std::queue<LoadedTableData>& loadedTableQueue,
                           std::mutex& mutex,
                           std::atomic<bool>& isTableLoading,
                           const std::string& vpsdbImageCacheDir)
{
    LOG_DEBUG("Starting background load for index: " + std::to_string(index));
    LoadedTableData data;
    data.index = index;

    try {
        // --- FIX: Handle both Array (normalized) and Object (raw) inputs ---
        const nlohmann::json* tablesPtr = nullptr;

        if (vpsdbJson.is_array()) {
            // Case A: vpsdbJson is already the array (Result of VpsDatabaseLoader logic)
            tablesPtr = &vpsdbJson;
        }
        else if (vpsdbJson.is_object() && vpsdbJson.contains("tables")) {
            // Case B: vpsdbJson is the raw root object
            tablesPtr = &vpsdbJson["tables"];
        }
        else {
            throw std::runtime_error("VPSDB JSON is neither an array nor a valid root object.");
        }

        const nlohmann::json& tables = *tablesPtr;

        if (index >= tables.size()) {
            throw std::out_of_range("Index out of range: " + std::to_string(index) +
                                    " (Total tables: " + std::to_string(tables.size()) + ")");
        }

        auto entry = tables[index];
        PinballTable table;

        // --- FULL PARSING LOGIC STARTS HERE ---

        // 1. Basic Fields
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

        // 2. tableFiles (Iterated array)
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

        // 3. b2sFiles (Iterated array)
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

        // 4. wheelArtFiles (Iterated array)
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

        // 5. topperFiles (Iterated array)
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

        // --- FULL PARSING LOGIC ENDS HERE ---

        data.table = table;

    } catch (const std::exception& e) {
        // Handle any parsing or index errors
        LOG_ERROR("Failed to load table at index " + std::to_string(index) + ": " + std::string(e.what()));
        std::lock_guard<std::mutex> lock(mutex);
        isTableLoading = false;
        return;
    }

    // The rest of the function (image loading and queue push) remains the same

    if (data.table.id.empty()) {
        std::lock_guard<std::mutex> lock(mutex);
        isTableLoading = false;
        LOG_ERROR("Failed to load table for index: " + std::to_string(index) + ", empty ID");
        return;
    }

    LOG_DEBUG("Loaded table data for index: " + std::to_string(index) + ", name: " + data.table.name);

    std::string backglassUrl;
    std::string playfieldUrl;

    if (!data.table.b2sFiles.empty()) {
        backglassUrl = data.table.b2sFiles[0].imgUrl;
        LOG_DEBUG("Backglass URL: " + backglassUrl);
    }

    if (!data.table.tableFiles.empty()) {
        playfieldUrl = data.table.tableFiles[0].imgUrl;
        LOG_DEBUG("Playfield URL: " + playfieldUrl);
    }

    fs::path cachePath = vpsdbImageCacheDir;
    fs::create_directories(cachePath);

    LOG_DEBUG("Cache dir = " + cachePath.string());

    // =========================
    // Utility lambdas
    // =========================
    auto fileOK = [&](const std::string& path) {
        return fs::exists(path) && fs::file_size(path) > 0;
    };

    // =========================
    // Backglass
    // =========================
    if (!backglassUrl.empty()) {
        data.backglassPath = (cachePath / (data.table.id + "_backglass.webp")).string();

        bool cached = fileOK(data.backglassPath);

        LOG_DEBUG("Backglass file = " + data.backglassPath +
                    " | exists=" + std::to_string(fs::exists(data.backglassPath)) +
                    " | size=" + (fs::exists(data.backglassPath) ? std::to_string(fs::file_size(data.backglassPath)) : "0"));

        if (cached) {
            LOG_DEBUG("Backglass cached, skipping download.");
        } else {
            LOG_DEBUG("Downloading BACKGLASS → " + backglassUrl);
            if (!VpsdbImage::downloadImage(backglassUrl, data.backglassPath)) {
                LOG_ERROR("Backglass download failed, clearing path.");
                data.backglassPath.clear();
            }
        }
    }

    // =========================
    // Playfield
    // =========================
    if (!playfieldUrl.empty()) {
        data.playfieldPath = (cachePath / (data.table.id + "_playfield.webp")).string();

        bool cached = fileOK(data.playfieldPath);

        LOG_DEBUG("Playfield file = " + data.playfieldPath +
                    " | exists=" + std::to_string(fs::exists(data.playfieldPath)) +
                    " | size=" + (fs::exists(data.playfieldPath) ? std::to_string(fs::file_size(data.playfieldPath)) : "0"));

        if (cached) {
            LOG_DEBUG("Playfield cached, skipping download.");
        } else {
            LOG_DEBUG("Downloading PLAYFIELD → " + playfieldUrl);
            if (!VpsdbImage::downloadImage(playfieldUrl, data.playfieldPath)) {
                LOG_ERROR("Playfield download failed, clearing path.");
                data.playfieldPath.clear();
            }
        }
    }

    // =========================
    // Push into queue
    // =========================
    {
        std::lock_guard<std::mutex> lock(mutex);
        loadedTableQueue.push(std::move(data));
        LOG_DEBUG("Enqueued table data for index: " + std::to_string(index));
    }

    LOG_DEBUG("Background load complete for index: " + std::to_string(index));
    isTableLoading = false;
}

// namespace vpsdb


} // namespace vpsdb
