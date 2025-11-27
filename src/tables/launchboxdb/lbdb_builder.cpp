// src/tables/launchboxdb/lbdb_builder.cpp
#include "lbdb_builder.h"
#include "log/logging.h"
#include <nlohmann/json.hpp>
#include <pugixml.hpp>
#include <curl/curl.h>
#include <zip.h>
#include <filesystem>
#include <fstream>
#include <vector>
#include <string>
#include <iostream>

using json = nlohmann::json;
namespace fs = std::filesystem;

static size_t WriteCallback(void* ptr, size_t size, size_t nmemb, std::string* data) {
    data->append((char*)ptr, size * nmemb);
    return size * nmemb;
}

class PinballExtractor : public pugi::xml_tree_walker {
    json& games;
    std::unordered_map<std::string, json> images;
    std::unordered_map<std::string, std::vector<std::string>> alt_names;
    std::unordered_map<std::string, json> game_data;

public:
    explicit PinballExtractor(json& g) : games(g) {
        LOG_INFO("PinballExtractor started");
    }

    bool for_each(pugi::xml_node& node) override {
        const char* name = node.name();

        if (strcmp(name, "GameImage") == 0) {
            std::string id   = node.child("DatabaseID").text().as_string();
            std::string type = node.child("Type").text().as_string();
            std::string file = node.child("FileName").text().as_string();

            if (!id.empty() && !file.empty() &&
                (type.find("Clear Logo") != std::string::npos ||
                 type.find("Flyer") != std::string::npos ||
                 type.find("Controls Information") != std::string::npos)) {
                images[id][type].push_back(file);
            }
        }
        else if (strcmp(name, "GameAlternateName") == 0) {
            std::string id  = node.child("DatabaseID").text().as_string();
            std::string alt = node.child("AlternateName").text().as_string();
            if (!id.empty() && !alt.empty()) {
                alt_names[id].push_back(alt);
            }
        }
        else if (strcmp(name, "Game") == 0) {
            if (strcmp(node.child("Platform").text().as_string(), "Pinball") != 0)
                return true;

            std::string id = node.child("DatabaseID").text().as_string();
            if (id.empty()) return true;

            json j;
            j["Id"]        = id;
            j["Name"]      = node.child("Name").text().as_string();
            j["Year"]      = node.child("ReleaseYear").text().as_string();
            j["Developer"] = node.child("Developer").text().as_string();
            j["Publisher"] = node.child("Publisher").text().as_string();

            game_data[id] = std::move(j);
        }
        return true;
    }

    ~PinballExtractor() {
        LOG_INFO("Merging pinball games with images and alt names...");

        size_t with_images = 0;
        size_t with_alt_names = 0;

        for (auto& [id, game] : game_data) {
            if (images.count(id)) {
                game["images"] = std::move(images[id]);
                images.erase(id);
                with_images++;
            }
            if (alt_names.count(id)) {
                game["altNames"] = std::move(alt_names[id]);
                alt_names.erase(id);
                with_alt_names++;
            }
            games.push_back(std::move(game));
        }

        LOG_INFO("Done!");
    }
};

namespace launchbox {

bool build_pinball_database(const Settings& settings, std::function<void(int,int)> progress) {
    const fs::path cache_dir = settings.mainCacheDir;  // "data/cache"
    const fs::path zip_path = settings.lbdbZipPath;    // "data/cache/Metadata.zip";
    const fs::path out_path = settings.lbdbPath;       // "data/cache/launchbox_pinball.json";

    fs::create_directories(cache_dir);

    LOG_INFO("Starting LaunchBox pinball DB build...");

    if (!fs::exists(zip_path)) {
        LOG_INFO("Downloading Metadata.zip (~400MB)...");

        CURL* curl = curl_easy_init();
        std::string buffer;
        curl_easy_setopt(curl, CURLOPT_URL, settings.lbdbZipUrl.c_str()); // "https://gamesdb.launchbox-app.com/Metadata.zip";
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buffer);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);

        struct { std::function<void(int,int)> cb; } userdata{ progress };
        curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION,
            [](void* p, curl_off_t total, curl_off_t now, curl_off_t, curl_off_t) -> int {
                auto* ud = (decltype(&userdata))p;
                if (total > 0 && ud->cb) ud->cb((int)now, (int)total);
                return 0;
            });
        curl_easy_setopt(curl, CURLOPT_XFERINFODATA, &userdata);

        CURLcode res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);
        if (res != CURLE_OK) {
            LOG_ERROR("Download failed");
            return false;
        }

        std::ofstream(zip_path, std::ios::binary).write(buffer.data(), buffer.size());
        LOG_INFO("Download complete.");
    } else {
        LOG_INFO("Metadata.zip already exists — skipping download");
        if (progress) progress(1, 3); // we’re past stage 1
    }
    // STAGE 1 COMPLETE
    if (progress) progress(1, 3);

    LOG_INFO("Extracting Metadata.xml...");
    int err = 0;
    zip_t* zip = zip_open(zip_path.string().c_str(), ZIP_RDONLY, &err);
    if (!zip) {
        LOG_ERROR("Failed to open ZIP");
        return false;
    }
    zip_file_t* zf = zip_fopen(zip, "Metadata.xml", 0);
    if (!zf) {
        zip_close(zip);
        LOG_ERROR("Metadata.xml not found in ZIP");
        return false;
    }

    std::string xml;
    char buf[8192];
    zip_int64_t bytes;
    while ((bytes = zip_fread(zf, buf, sizeof(buf))) > 0)
        xml.append(buf, bytes);

    zip_fclose(zf);
    zip_close(zip);

    // STAGE 2 START
    LOG_INFO("Parsing XML (this may take 30-60 seconds)...");
    if (progress) progress(2, 3);

    json games = json::array();
    // wrap parsing so walker destructor merges
    {
        PinballExtractor walker(games);

        pugi::xml_document doc;
        if (!doc.load_string(xml.c_str())) {
            LOG_ERROR("Failed to parse XML");
            return false;
        }

        doc.traverse(walker);
    }
    LOG_INFO("Saving games");
    std::ofstream(out_path) << games.dump(2);

    // STAGE 3 COMPLETE
    if (progress) progress(3, 3);

    // SUCCESS — safe to delete 400MB zip
    try {
        fs::remove(zip_path);
        LOG_INFO("Deleted Metadata.zip to save disk space.");
    } catch (...) {
        LOG_WARN("Could not delete Metadata.zip — manual cleanup recommended.");
    }

    LOG_INFO("LaunchBox pinball DB build complete!");
    return true;
}

} // namespace launchbox

// Metadata.xml
// game example (fields we care about)

// <Game>
//     <Name>James Bond 007 (Gottlieb)</Name> //for matchmaking
//     <ReleaseYear>1980</ReleaseYear> //for matchmaking
//     <Overview>Gottlieb’s James Bond 007 ...</Overview> //another source of description/comment for table data.
//     <DatabaseID>193774</DatabaseID> //to get images (and save to table_data the id)
//     <Platform>Pinball</Platform> //to filter pinball tables only in json
//     <Developer>Gottlieb</Developer> //for matchmaking
//     <Publisher>Gottlieb</Publisher> //for matchmaking
// </Game>



// Image example. each field (ex clear logo) might have more than one logo option...

// <GameImage>

//     <DatabaseID>463784</DatabaseID> //to relate to the matched table
//     <FileName>80d497d8-0a53-4e8d-ab0f-d79edccd0bd6.png</FileName> //the filename to download<a href="https://images.launchbox-app.com/<filename>" target="_blank" rel="noopener noreferrer nofollow"></a>
//     <Type>Clear Logo</Type> //to save the correct type (for DMD logo here)
//or // <Type>Advertisement Flyer - Front</Type> // flyer front
//or // <Type>Advertisement Flyer - Back</Type> // flyer back
//or // <Type>Arcade - Controls Information</Type> // game rules (for pinball)

// </GameImage>




// game alternate names for better matching.

// <GameAlternateName>

    // <AlternateName>Asuncia: Staff of Sealing</AlternateName>
    // <DatabaseID>153217</DatabaseID>

// </GameAlternateName>
