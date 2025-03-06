// main.cpp
// ASAP-CABINET-FE in C++/SDL2
// - Scans VPX_TABLES_PATH recursively for .vpx files
// - For each table, loads filename. (videos)/images for the playfield, wheel, backglass and DMD
// - Creates two windows: primary (table name/wheel/table playfield)
//                    and secondary (backglass with dmd)
// - Uses left/right arrow/shift keys to change tables with a fade transition
// - Press Enter to launch the table via vpinballx_gl, 'q'/esc to quit
// - Uses VLC for video playback
// - Handles video context cleanup and setup
// - Loads textures with fallback options
// - Renders text using SDL_ttf
// - Settings are done via config.ini file.
// Dependencies:
// sudo apt-get install -y build-essential libsdl2-dev libsdl2-image-dev libsdl2-ttf-dev libsdl2-mixer-dev libvlc-dev
// Compile:
// g++ main.cpp -std=c++17 -I/usr/include/SDL2 -D_REENTRANT -lSDL2 -lSDL2_image -lSDL2_ttf -lSDL2_mixer -lvlc -o ASAPCabinetFE
// Author: Tarso Galvão Mar/2025 | github.com/surtarso/ASAPCabinetFE

#include <algorithm>    // For std::sort
#include <SDL.h>        // For SDL main library
#include <SDL_image.h>  // For SDL image loading
#include <SDL_ttf.h>    // For SDL font rendering
#include <SDL_mixer.h>  // For SDL audio mixing
#include <vlc/vlc.h>    // For VLC video playback
#include <iostream>     // For standard I/O operations
#include <filesystem>   // For filesystem operations
#include <vector>       // For std::vector
#include <string>       // For std::string
#include <cstdlib>      // For std::system
#include <fstream>      // For file I/O operations
#include <map>          // For std::map container


namespace fs = std::filesystem; // Alias for filesystem lib

// ------------------ Configuration Constants ------------------

// Configuration Variables (no longer const)
std::string VPX_TABLES_PATH;
std::string VPX_EXECUTABLE_CMD;
std::string VPX_SUB_CMD;         // Not in config.ini, so we'll set a default later
std::string VPX_START_ARGS;
std::string VPX_END_ARGS;

std::string DEFAULT_TABLE_IMAGE;     // Not in config.ini, set default later
std::string DEFAULT_BACKGLASS_IMAGE; // Not in config.ini
std::string DEFAULT_DMD_IMAGE;       // Not in config.ini
std::string DEFAULT_WHEEL_IMAGE;     // Not in config.ini
std::string DEFAULT_TABLE_VIDEO;     // Not in config.ini
std::string DEFAULT_BACKGLASS_VIDEO; // Not in config.ini
std::string DEFAULT_DMD_VIDEO;       // Not in config.ini

std::string CUSTOM_TABLE_IMAGE;
std::string CUSTOM_BACKGLASS_IMAGE;
std::string CUSTOM_DMD_IMAGE;
std::string CUSTOM_WHEEL_IMAGE;
std::string CUSTOM_TABLE_VIDEO;
std::string CUSTOM_BACKGLASS_VIDEO;
std::string CUSTOM_DMD_VIDEO;

int MAIN_WINDOW_MONITOR;
int MAIN_WINDOW_WIDTH;
int MAIN_WINDOW_HEIGHT;
int WHEEL_IMAGE_SIZE;
int WHEEL_IMAGE_MARGIN;
std::string FONT_PATH;
int FONT_SIZE;

int SECOND_WINDOW_MONITOR;
int SECOND_WINDOW_WIDTH;
int SECOND_WINDOW_HEIGHT;
int BACKGLASS_MEDIA_WIDTH;
int BACKGLASS_MEDIA_HEIGHT;
int DMD_MEDIA_WIDTH;
int DMD_MEDIA_HEIGHT;

int FADE_DURATION_MS;         // Not in config.ini
Uint8 FADE_TARGET_ALPHA;      // Not in config.ini
std::string TABLE_CHANGE_SOUND; // Not in config.ini
std::string TABLE_LOAD_SOUND;   // Not in config.ini

// ------------------ Data Structures ------------------

// Structure to hold information about each pinball table
struct Table {
    std::string tableName;       // Name of the table
    std::string vpxFile;         // Path to the .vpx file
    std::string folder;          // Folder containing the table's assets
    std::string tableImage;      // Path to the table image
    std::string wheelImage;      // Path to the wheel image
    std::string backglassImage;  // Path to the backglass image
    std::string dmdImage;        // Path to the DMD image
    std::string tableVideo;      // Path to the table video
    std::string backglassVideo;  // Path to the backglass video
    std::string dmdVideo;        // Path to the DMD video
};

// Structure to hold video context information for VLC video playback
struct VideoContext {
    SDL_Texture* texture;  // SDL texture to render the video frame
    Uint8* pixels;         // Pointer to the pixel data of the video frame
    int pitch;             // Number of bytes in a row of pixel data
    SDL_mutex* mutex;      // Mutex to synchronize access to the pixel data
    int width;             // Width of the video frame
    int height;            // Height of the video frame
    bool updated;          // Flag to indicate if the video frame has been updated
};

// ------------------ Utility Functions ------------------
// Get image path with fallback
std::string getImagePath(const std::string &root, const std::string &imagePath, const std::string &defaultImagePath) {
    fs::path imageFile = fs::path(root) / imagePath;
    if (fs::exists(imageFile))
        return imageFile.string();
    return defaultImagePath;
}

// Get video path with fallback
std::string getVideoPath(const std::string &root, const std::string &videoPath, const std::string &defaultVideoPath) {
    fs::path videoFile = fs::path(root) / videoPath;
    if (fs::exists(videoFile))
        return videoFile.string();
    else if (fs::exists(defaultVideoPath))
        return defaultVideoPath;
    else
        return "";
}

// Load table list
std::vector<Table> loadTableList() {
    std::vector<Table> tables;
    for (const auto &entry : fs::recursive_directory_iterator(VPX_TABLES_PATH)) {
        if (entry.is_regular_file() && entry.path().extension() == ".vpx") {
            Table table;
            table.vpxFile = entry.path().string();
            table.folder = entry.path().parent_path().string();
            table.tableName = entry.path().stem().string();
            table.tableImage     = getImagePath(table.folder, CUSTOM_TABLE_IMAGE, DEFAULT_TABLE_IMAGE);
            table.wheelImage     = getImagePath(table.folder, CUSTOM_WHEEL_IMAGE, DEFAULT_WHEEL_IMAGE);
            table.backglassImage = getImagePath(table.folder, CUSTOM_BACKGLASS_IMAGE, DEFAULT_BACKGLASS_IMAGE);
            table.dmdImage       = getImagePath(table.folder, CUSTOM_DMD_IMAGE, DEFAULT_DMD_IMAGE);
            table.tableVideo     = getVideoPath(table.folder, CUSTOM_TABLE_VIDEO, DEFAULT_TABLE_VIDEO);
            table.backglassVideo = getVideoPath(table.folder, CUSTOM_BACKGLASS_VIDEO, DEFAULT_BACKGLASS_VIDEO);
            table.dmdVideo       = getVideoPath(table.folder, CUSTOM_DMD_VIDEO, DEFAULT_DMD_VIDEO);
            tables.push_back(table);
        }
    }
    std::sort(tables.begin(), tables.end(), [](const Table &a, const Table &b) {
        return a.tableName < b.tableName;
    });
    return tables;
}

// Load images texture with fallback
SDL_Texture* loadTexture(SDL_Renderer* renderer, const std::string &path, const std::string &fallbackPath) {
    SDL_Texture* tex = IMG_LoadTexture(renderer, path.c_str());
    if (!tex) {
        std::cerr << "Failed to load " << path << ". Using fallback." << std::endl;
        tex = IMG_LoadTexture(renderer, fallbackPath.c_str());
    }
    return tex;
}

// Load font
SDL_Texture* renderText(SDL_Renderer* renderer, TTF_Font* font, const std::string &message, SDL_Color color, SDL_Rect &textRect) {
    SDL_Surface* surf = TTF_RenderUTF8_Blended(font, message.c_str(), color);
    if (!surf) {
        std::cerr << "TTF_RenderUTF8_Blended error: " << TTF_GetError() << std::endl;
        return nullptr;
    }
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surf);
    textRect.w = surf->w;
    textRect.h = surf->h;
    SDL_FreeSurface(surf);
    return texture;
}

// ----------------- Handle video playback (vlc) -----------------

// Locks the video context mutex and provides access to the pixel data.
void* lock(void* data, void** pixels) {
    VideoContext* ctx = static_cast<VideoContext*>(data);
    if (!ctx || !ctx->mutex) return nullptr;
    SDL_LockMutex(ctx->mutex);
    *pixels = ctx->pixels;
    return nullptr;
}

// Unlocks the video context and updates its state.
void unlock(void* data, void* id, void* const* pixels) {
    VideoContext* ctx = static_cast<VideoContext*>(data);
    if (ctx) {
        ctx->updated = true;
        SDL_UnlockMutex(ctx->mutex);
    }
}

// Empty function needed for display operations.
void display(void* data, void* id) {}

// Cleans up the resources associated with a VideoContext and libvlc_media_player_t.
void cleanupVideoContext(VideoContext& ctx, libvlc_media_player_t*& player) {
    if (player) {
        libvlc_media_player_stop(player);
        libvlc_media_player_release(player);
        player = nullptr;
    }
    if (ctx.texture) {
        SDL_DestroyTexture(ctx.texture);
        ctx.texture = nullptr;
    }
    if (ctx.pixels) {
        delete[] ctx.pixels;
        ctx.pixels = nullptr;
    }
    if (ctx.mutex) {
        SDL_DestroyMutex(ctx.mutex);
        ctx.mutex = nullptr;
    }
}

/**
 * @brief Sets up a video player using libVLC and SDL.
 * 
 * This function initializes a video player with the given VLC instance and SDL renderer, 
 * loads the specified video file, and prepares the video context for rendering.
 * 
 * @param vlcInstance A pointer to the libVLC instance.
 * @param renderer    A pointer to the SDL renderer.
 * @param videoPath   The file path to the video to be played.
 * @param ctx         A reference to the VideoContext structure to hold video rendering data.
 * @param width       The width of the video frame.
 * @param height      The height of the video frame.
 * @return A pointer to the initialized libvlc_media_player_t, or nullptr if an error occurs.
 */
 libvlc_media_player_t* setupVideoPlayer(libvlc_instance_t* vlcInstance, SDL_Renderer* renderer, 
                                      const std::string& videoPath, VideoContext& ctx, int width, int height) {
    libvlc_media_t* media = libvlc_media_new_path(vlcInstance, videoPath.c_str());
    if (!media) {
        std::cerr << "Failed to create media for " << videoPath << std::endl;
        return nullptr;
    }

    libvlc_media_add_option(media, "input-repeat=65535"); // Loop indefinitely

    libvlc_media_player_t* player = libvlc_media_player_new_from_media(media);
    libvlc_media_release(media);
    if (!player) {
        std::cerr << "Failed to create media player for " << videoPath << std::endl;
        return nullptr;
    }

    ctx.texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, width, height);
    if (!ctx.texture) {
        std::cerr << "Failed to create texture: " << SDL_GetError() << std::endl;
        libvlc_media_player_release(player);
        return nullptr;
    }

    ctx.pixels = new (std::nothrow) Uint8[width * height * 4]; // BGRA: 4 bytes per pixel
    if (!ctx.pixels) {
        std::cerr << "Failed to allocate video buffer" << std::endl;
        SDL_DestroyTexture(ctx.texture);
        libvlc_media_player_release(player);
        return nullptr;
    }

    ctx.pitch = width * 4;           // Set the pitch (number of bytes in a row of pixel data) to width * 4 (BGRA format)
    ctx.mutex = SDL_CreateMutex();   // Create a mutex to synchronize access to the pixel data
    ctx.width = width;               // Set the width of the video frame
    ctx.height = height;             // Set the height of the video frame
    ctx.updated = false;             // Initialize the updated flag to false

    libvlc_video_set_callbacks(player, lock, unlock, display, &ctx);
    libvlc_video_set_format(player, "BGRA", width, height, width * 4);

    if (libvlc_media_player_play(player) < 0) {
        std::cerr << "Failed to play video: " << videoPath << std::endl;
        cleanupVideoContext(ctx, player);
        return nullptr;
    }

    SDL_Delay(100); // Wait for VLC to initialize
    return player;
}

// ---------------- Launch Table -----------------------

void launchTable(const Table &table) {
    std::string command = VPX_START_ARGS + " " + VPX_EXECUTABLE_CMD + " " + 
                          VPX_SUB_CMD + " \"" + table.vpxFile + "\" " + VPX_END_ARGS;
    std::cout << "Launching: " << command << std::endl;
    std::system(command.c_str());
}

// -------------------- Settings --------------------------

// Helper functions to get values with defaults
std::string get_string(const std::map<std::string, std::map<std::string, std::string>>& config, 
        const std::string& section, const std::string& key, const std::string& default_value) {
    if (config.count(section) && config.at(section).count(key)) {
        return config.at(section).at(key);
    }
    return default_value;
}

int get_int(const std::map<std::string, std::map<std::string, std::string>>& config, 
        const std::string& section, const std::string& key, int default_value) {
    if (config.count(section) && config.at(section).count(key)) {
        try {
            return std::stoi(config.at(section).at(key));
        } catch (const std::exception&) {
            return default_value;
        }
    }
    return default_value;
}

// Structure to hold all config data
std::map<std::string, std::map<std::string, std::string>> load_config(const std::string& filename) {
    std::map<std::string, std::map<std::string, std::string>> config;
    std::ifstream file(filename);
    std::string current_section;

    if (!file.is_open()) {
        std::cerr << "Could not open " << filename << ". Using defaults." << std::endl;
        return config;
    }

    std::string line;
    while (std::getline(file, line)) {
        // Skip empty lines or comments
        if (line.empty() || line[0] == ';') continue;

        // Check for section (e.g., [VPX])
        if (line[0] == '[') {
            size_t end = line.find(']');
            if (end != std::string::npos) {
                current_section = line.substr(1, end - 1);
                config[current_section]; // Create section if it doesn't exist
            }
            continue;
        }

        // Parse key=value pairs
        size_t eq_pos = line.find('=');
        if (eq_pos != std::string::npos && !current_section.empty()) {
            std::string key = line.substr(0, eq_pos);
            std::string value = line.substr(eq_pos + 1);
            // Remove any trailing or leading whitespace (basic cleanup)
            key.erase(0, key.find_first_not_of(" \t"));
            key.erase(key.find_last_not_of(" \t") + 1);
            value.erase(0, value.find_first_not_of(" \t"));
            value.erase(value.find_last_not_of(" \t") + 1);
            config[current_section][key] = value;
        }
    }
    file.close();
    return config;
}

// ------------------ Main Application ------------------

enum class TransitionState { IDLE, FADING_OUT, FADING_IN };

// Main function for initializing and running the SDL application
/**
 * @brief Main function for initializing and running the SDL application.
 * 
 * This function initializes SDL, SDL_image, SDL_ttf, SDL_mixer, and libVLC libraries.
 * It creates primary and secondary SDL windows and renderers, loads resources such as fonts and sounds,
 * and handles the main event loop for user interactions.
 * 
 * @param argc The number of command-line arguments.
 * @param argv The array of command-line arguments.
 * @return int Returns 0 on successful execution, or 1 on failure.
 */
int main(int argc, char* argv[]) {
    // ------------- Load the configuration --------------
    auto config = load_config("config.ini");

    // Assign VPX settings
    VPX_TABLES_PATH    = get_string(config, "VPX", "TablesPath", "/home/tarso/Games/vpinball/build/tables/");
    VPX_EXECUTABLE_CMD = get_string(config, "VPX", "ExecutableCmd", "/home/tarso/Games/vpinball/build/VPinballX_GL");
    VPX_SUB_CMD        = "-Play"; // Not in config, use hardcoded default
    VPX_START_ARGS     = get_string(config, "VPX", "StartArgs", "DRI_PRIME=1 gamemoderun");
    VPX_END_ARGS       = get_string(config, "VPX", "EndArgs", "");

    // Assign CustomMedia settings
    CUSTOM_TABLE_IMAGE     = get_string(config, "CustomMedia", "TableImage", "images/table.png");
    CUSTOM_BACKGLASS_IMAGE = get_string(config, "CustomMedia", "BackglassImage", "images/backglass.png");
    CUSTOM_DMD_IMAGE       = get_string(config, "CustomMedia", "DmdImage", "images/marquee.png");
    CUSTOM_WHEEL_IMAGE     = get_string(config, "CustomMedia", "WheelImage", "images/wheel.png");
    CUSTOM_TABLE_VIDEO     = get_string(config, "CustomMedia", "TableVideo", "video/table.mp4");
    CUSTOM_BACKGLASS_VIDEO = get_string(config, "CustomMedia", "BackglassVideo", "video/backglass.mp4");
    CUSTOM_DMD_VIDEO       = get_string(config, "CustomMedia", "DmdVideo", "video/dmd.mp4");

    // Assign WindowSettings
    MAIN_WINDOW_MONITOR   = get_int(config, "WindowSettings", "MainMonitor", 1);
    MAIN_WINDOW_WIDTH     = get_int(config, "WindowSettings", "MainWidth", 1080);
    MAIN_WINDOW_HEIGHT    = get_int(config, "WindowSettings", "MainHeight", 1920);
    SECOND_WINDOW_MONITOR = get_int(config, "WindowSettings", "SecondMonitor", 0);
    SECOND_WINDOW_WIDTH   = get_int(config, "WindowSettings", "SecondWidth", 1024);
    SECOND_WINDOW_HEIGHT  = get_int(config, "WindowSettings", "SecondHeight", 1024);

    // Assign Font settings
    FONT_PATH = get_string(config, "Font", "Path", "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf");
    FONT_SIZE = get_int(config, "Font", "Size", 28);

    // Assign MediaDimensions
    WHEEL_IMAGE_SIZE      = get_int(config, "MediaDimensions", "WheelImageSize", 300);
    WHEEL_IMAGE_MARGIN    = get_int(config, "MediaDimensions", "WheelImageMargin", 24);
    BACKGLASS_MEDIA_WIDTH  = get_int(config, "MediaDimensions", "BackglassWidth", 1024);
    BACKGLASS_MEDIA_HEIGHT = get_int(config, "MediaDimensions", "BackglassHeight", 768);
    DMD_MEDIA_WIDTH        = get_int(config, "MediaDimensions", "DmdWidth", 1024);
    DMD_MEDIA_HEIGHT       = get_int(config, "MediaDimensions", "DmdHeight", 256);

    // Set defaults for variables not in config.ini
    DEFAULT_TABLE_IMAGE     = "img/default_table.png";
    DEFAULT_BACKGLASS_IMAGE = "img/default_backglass.png";
    DEFAULT_DMD_IMAGE       = "img/default_dmd.png";
    DEFAULT_WHEEL_IMAGE     = "img/default_wheel.png";
    DEFAULT_TABLE_VIDEO     = "img/default_table.mp4";
    DEFAULT_BACKGLASS_VIDEO = "img/default_backglass.mp4";
    DEFAULT_DMD_VIDEO       = "img/default_dmd.mp4";
    FADE_DURATION_MS        = 300;
    FADE_TARGET_ALPHA       = 128;
    TABLE_CHANGE_SOUND      = "snd/table_change.mp3";
    TABLE_LOAD_SOUND        = "snd/table_load.mp3";

    // ------------------ Initialization ------------------
    // ----- SDL init
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_AUDIO) != 0) {
        std::cerr << "SDL_Init Error: " << SDL_GetError() << std::endl;
        return 1;
    }

    // ----- Images
    if (!(IMG_Init(IMG_INIT_PNG | IMG_INIT_JPG) & (IMG_INIT_PNG | IMG_INIT_JPG))) {
        std::cerr << "IMG_Init Error: " << IMG_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    // ----- Font
    if (TTF_Init() != 0) {
        std::cerr << "TTF_Init Error: " << TTF_GetError() << std::endl;
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    // ----- Audio player
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
        std::cerr << "SDL_mixer Error: " << Mix_GetError() << std::endl;
        TTF_Quit();
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    // ----- Videos (vlc)
    libvlc_instance_t* vlcInstance = libvlc_new(0, nullptr);
    if (!vlcInstance) {
        std::cerr << "Failed to initialize VLC instance." << std::endl;
        Mix_CloseAudio();
        TTF_Quit();
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    // ----- Primary Window
    SDL_Window* primaryWindow = SDL_CreateWindow("Playfield",
        SDL_WINDOWPOS_CENTERED_DISPLAY(MAIN_WINDOW_MONITOR), SDL_WINDOWPOS_CENTERED,
        MAIN_WINDOW_WIDTH, MAIN_WINDOW_HEIGHT, SDL_WINDOW_SHOWN | SDL_WINDOW_BORDERLESS);
    if (!primaryWindow) {
        std::cerr << "Failed to create primary window: " << SDL_GetError() << std::endl;
        libvlc_release(vlcInstance);
        Mix_CloseAudio();
        TTF_Quit();
        IMG_Quit();
        SDL_Quit();
        return 1;
    }
    
    SDL_Renderer* primaryRenderer = SDL_CreateRenderer(primaryWindow, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!primaryRenderer) {
        std::cerr << "Failed to create primary renderer: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(primaryWindow);
        libvlc_release(vlcInstance);
        Mix_CloseAudio();
        TTF_Quit();
        IMG_Quit();
        SDL_Quit();
        return 1;
    }
    
    // ----- Secondary Window
    SDL_Window* secondaryWindow = SDL_CreateWindow("Backglass",
        SDL_WINDOWPOS_CENTERED_DISPLAY(SECOND_WINDOW_MONITOR), SDL_WINDOWPOS_CENTERED,
        SECOND_WINDOW_WIDTH, SECOND_WINDOW_HEIGHT, SDL_WINDOW_SHOWN | SDL_WINDOW_BORDERLESS);
    if (!secondaryWindow) {
        std::cerr << "Failed to create secondary window: " << SDL_GetError() << std::endl;
        SDL_DestroyRenderer(primaryRenderer);
        SDL_DestroyWindow(primaryWindow);
        libvlc_release(vlcInstance);
        Mix_CloseAudio();
        TTF_Quit();
        IMG_Quit();
        SDL_Quit();
        return 1;
    }
    
    SDL_Renderer* secondaryRenderer = SDL_CreateRenderer(secondaryWindow, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!secondaryRenderer) {
        std::cerr << "Failed to create secondary renderer: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(secondaryWindow);
        SDL_DestroyRenderer(primaryRenderer);
        SDL_DestroyWindow(primaryWindow);
        libvlc_release(vlcInstance);
        Mix_CloseAudio();
        TTF_Quit();
        IMG_Quit();
        SDL_Quit();
        return 1;
    }
    
    // ----- load Fonts
    TTF_Font* font = TTF_OpenFont(FONT_PATH.c_str(), FONT_SIZE);
    if (!font) {
        std::cerr << "Failed to load font: " << TTF_GetError() << std::endl;
    }
    
    // ----- load Sounds
    Mix_Chunk* tableChangeSound = Mix_LoadWAV(TABLE_CHANGE_SOUND.c_str());
    if (!tableChangeSound) {
        std::cerr << "Mix_LoadWAV Error: " << Mix_GetError() << std::endl;
    }
    Mix_Chunk* tableLoadSound = Mix_LoadWAV(TABLE_LOAD_SOUND.c_str());
    if (!tableLoadSound) {
        std::cerr << "Mix_LoadWAV Error: " << Mix_GetError() << std::endl;
    }

    // ----- load Table List
    std::vector<Table> tables = loadTableList();
    if (tables.empty()) {
        std::cerr << "No .vpx files found in " << VPX_TABLES_PATH << std::endl;
        if (font) TTF_CloseFont(font);
        if (tableChangeSound) Mix_FreeChunk(tableChangeSound);
        if (tableLoadSound) Mix_FreeChunk(tableLoadSound);
        SDL_DestroyRenderer(secondaryRenderer);
        SDL_DestroyWindow(secondaryWindow);
        SDL_DestroyRenderer(primaryRenderer);
        SDL_DestroyWindow(primaryWindow);
        libvlc_release(vlcInstance);
        Mix_CloseAudio();
        TTF_Quit();
        IMG_Quit();
        SDL_Quit();
        return 1;
    }
    
    // ------------------ Texture and State Variables ------------------

    size_t currentIndex = 0;                 // Index of the currently selected table
    SDL_Texture* tableTexture = nullptr;     // Texture for the table image
    SDL_Texture* wheelTexture = nullptr;     // Texture for the wheel image
    SDL_Texture* backglassTexture = nullptr; // Texture for the backglass image
    SDL_Texture* dmdTexture = nullptr;       // Texture for the DMD image
    SDL_Texture* tableNameTexture = nullptr; // Texture for the table name text
    SDL_Rect tableNameRect = {0, 0, 0, 0};   // Rectangle for the table name text position and size

    // VLC media players for table, backglass, and DMD videos
    libvlc_media_player_t* tableVideoPlayer = nullptr;
    libvlc_media_player_t* backglassVideoPlayer = nullptr;
    libvlc_media_player_t* dmdVideoPlayer = nullptr;

    // Video contexts to hold texture and pixel data for rendering video frames
    VideoContext tableVideoCtx = {nullptr, nullptr, 0, nullptr, 0, 0, false};
    VideoContext backglassVideoCtx = {nullptr, nullptr, 0, nullptr, 0, 0, false};
    VideoContext dmdVideoCtx = {nullptr, nullptr, 0, nullptr, 0, 0, false};

    // ------------------ Load Current Table Textures ------------------
    /**
     * @brief Loads the current table textures and sets up video players if necessary.
     *
     * This function performs the following tasks:
     * - Cleans up existing video contexts and textures.
     * - Loads new textures or sets up video players for the table, backglass, and DMD based on the current table's data.
     * - Loads the wheel image texture.
     * - Renders the table name texture if a font is available.
     */
    auto loadCurrentTableTextures = [&]() {
        // Cleanup 
        cleanupVideoContext(tableVideoCtx, tableVideoPlayer);
        cleanupVideoContext(backglassVideoCtx, backglassVideoPlayer);
        cleanupVideoContext(dmdVideoCtx, dmdVideoPlayer);

        if (tableTexture) { SDL_DestroyTexture(tableTexture); tableTexture = nullptr; }
        if (backglassTexture) { SDL_DestroyTexture(backglassTexture); backglassTexture = nullptr; }
        if (dmdTexture) { SDL_DestroyTexture(dmdTexture); dmdTexture = nullptr; }
        if (tableNameTexture) { SDL_DestroyTexture(tableNameTexture); tableNameTexture = nullptr; }
        
        // Get the current table
        const Table &tbl = tables[currentIndex];

        // Load table media: prioritize custom video/image and fallback to default video/image
        if (!tbl.tableVideo.empty()) {
            tableVideoPlayer = setupVideoPlayer(vlcInstance, primaryRenderer, tbl.tableVideo, 
                                              tableVideoCtx, MAIN_WINDOW_WIDTH, MAIN_WINDOW_HEIGHT);
        } else {
            tableTexture = loadTexture(primaryRenderer, tbl.tableImage, DEFAULT_TABLE_IMAGE);
        }

        if (!tbl.backglassVideo.empty()) {
            backglassVideoPlayer = setupVideoPlayer(vlcInstance, secondaryRenderer, tbl.backglassVideo,
                                                  backglassVideoCtx, BACKGLASS_MEDIA_WIDTH, BACKGLASS_MEDIA_HEIGHT);
        } else {
            backglassTexture = loadTexture(secondaryRenderer, tbl.backglassImage, DEFAULT_BACKGLASS_IMAGE);
        }

        if (!tbl.dmdVideo.empty()) {
            dmdVideoPlayer = setupVideoPlayer(vlcInstance, secondaryRenderer, tbl.dmdVideo,
                                            dmdVideoCtx, DMD_MEDIA_WIDTH, DMD_MEDIA_HEIGHT);
        } else {
            dmdTexture = loadTexture(secondaryRenderer, tbl.dmdImage, DEFAULT_DMD_IMAGE);
        }

        // Load wheel image
        wheelTexture = loadTexture(primaryRenderer, tbl.wheelImage, DEFAULT_WHEEL_IMAGE);

        // Load Font
        if (font) {
            SDL_Color textColor = {255, 255, 255, 255};
            tableNameTexture = renderText(primaryRenderer, font, tbl.tableName, textColor, tableNameRect);
            tableNameRect.x = 10;
            tableNameRect.y = MAIN_WINDOW_HEIGHT - tableNameRect.h - 20;
        }
    };

    loadCurrentTableTextures();

    TransitionState transitionState = TransitionState::IDLE; // Initial transition state is idle
    Uint32 transitionStartTime = 0;         // Variable to store the start time of the transition
    bool quit = false;                      // Flag to indicate if the application should quit
    SDL_Event event;                        // SDL event structure to handle events

    while (!quit) {
        // --------------------- Handle Key presses ---------------------
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                quit = true;
            }
            else if (event.type == SDL_KEYDOWN && transitionState == TransitionState::IDLE) {
                // Move LEFT
                if (event.key.keysym.sym == SDLK_LEFT || event.key.keysym.sym == SDLK_LSHIFT) {
                    if (tableVideoPlayer) libvlc_media_player_stop(tableVideoPlayer);
                    if (backglassVideoPlayer) libvlc_media_player_stop(backglassVideoPlayer);
                    if (dmdVideoPlayer) libvlc_media_player_stop(dmdVideoPlayer);
                    if (tableChangeSound) Mix_PlayChannel(-1, tableChangeSound, 0);
                    currentIndex = (currentIndex + tables.size() - 1) % tables.size();
                    transitionState = TransitionState::FADING_OUT;
                    transitionStartTime = SDL_GetTicks();
                }
                // Move Right
                else if (event.key.keysym.sym == SDLK_RIGHT || event.key.keysym.sym == SDLK_RSHIFT) {
                    if (tableVideoPlayer) libvlc_media_player_stop(tableVideoPlayer);
                    if (backglassVideoPlayer) libvlc_media_player_stop(backglassVideoPlayer);
                    if (dmdVideoPlayer) libvlc_media_player_stop(dmdVideoPlayer);
                    if (tableChangeSound) Mix_PlayChannel(-1, tableChangeSound, 0);
                    currentIndex = (currentIndex + 1) % tables.size();
                    transitionState = TransitionState::FADING_OUT;
                    transitionStartTime = SDL_GetTicks();
                }
                // Launch Table
                else if (event.key.keysym.sym == SDLK_RETURN || event.key.keysym.sym == SDLK_KP_ENTER) {
                    if (tableLoadSound) Mix_PlayChannel(-1, tableLoadSound, 0);
                    launchTable(tables[currentIndex]);
                }
                // Quit
                else if (event.key.keysym.sym == SDLK_ESCAPE || event.key.keysym.sym == SDLK_q) {
                    quit = true;
                }
            }
        }

        // ------------------ Handle Transitions ------------------
        Uint8 currentAlpha = 255;
        Uint32 now = SDL_GetTicks();
        if (transitionState != TransitionState::IDLE) {
            Uint32 elapsed = now - transitionStartTime;
            int halfDuration = FADE_DURATION_MS / 2;
            if (transitionState == TransitionState::FADING_OUT) {
                if (elapsed < (Uint32)halfDuration) {
                    currentAlpha = 255 - (Uint8)(((255 - FADE_TARGET_ALPHA) * elapsed) / halfDuration);
                } else {
                    loadCurrentTableTextures();
                    transitionState = TransitionState::FADING_IN;
                    transitionStartTime = SDL_GetTicks();
                    currentAlpha = FADE_TARGET_ALPHA;
                }
            }
            else if (transitionState == TransitionState::FADING_IN) {
                if (elapsed < (Uint32)halfDuration) {
                    currentAlpha = FADE_TARGET_ALPHA + (Uint8)(((255 - FADE_TARGET_ALPHA) * elapsed) / halfDuration);
                } else {
                    currentAlpha = 255;
                    transitionState = TransitionState::IDLE;
                }
            }
        }

        if (tableTexture)               SDL_SetTextureAlphaMod(tableTexture, currentAlpha);
        if (wheelTexture)               SDL_SetTextureAlphaMod(wheelTexture, currentAlpha);
        if (backglassTexture)           SDL_SetTextureAlphaMod(backglassTexture, currentAlpha);
        if (dmdTexture)                 SDL_SetTextureAlphaMod(dmdTexture, currentAlpha);
        if (tableNameTexture)           SDL_SetTextureAlphaMod(tableNameTexture, currentAlpha);
        if (tableVideoCtx.texture)      SDL_SetTextureAlphaMod(tableVideoCtx.texture, currentAlpha);
        if (backglassVideoCtx.texture)  SDL_SetTextureAlphaMod(backglassVideoCtx.texture, currentAlpha);
        if (dmdVideoCtx.texture)        SDL_SetTextureAlphaMod(dmdVideoCtx.texture, currentAlpha);

        if (tableVideoPlayer && tableVideoCtx.texture && tableVideoCtx.updated && tableVideoCtx.pixels) {
            SDL_LockMutex(tableVideoCtx.mutex);
            SDL_UpdateTexture(tableVideoCtx.texture, nullptr, tableVideoCtx.pixels, tableVideoCtx.pitch);
            tableVideoCtx.updated = false;
            SDL_UnlockMutex(tableVideoCtx.mutex);
        }

        if (backglassVideoPlayer && backglassVideoCtx.texture && backglassVideoCtx.updated && backglassVideoCtx.pixels) {
            SDL_LockMutex(backglassVideoCtx.mutex);
            SDL_UpdateTexture(backglassVideoCtx.texture, nullptr, backglassVideoCtx.pixels, backglassVideoCtx.pitch);
            backglassVideoCtx.updated = false;
            SDL_UnlockMutex(backglassVideoCtx.mutex);
        }

        if (dmdVideoPlayer && dmdVideoCtx.texture && dmdVideoCtx.updated && dmdVideoCtx.pixels) {
            SDL_LockMutex(dmdVideoCtx.mutex);
            SDL_UpdateTexture(dmdVideoCtx.texture, nullptr, dmdVideoCtx.pixels, dmdVideoCtx.pitch);
            dmdVideoCtx.updated = false;
            SDL_UnlockMutex(dmdVideoCtx.mutex);
        }

        // -------------------------- Render Primary Screen -----------------------
        SDL_SetRenderDrawColor(primaryRenderer, 32, 32, 32, 255);
        SDL_RenderClear(primaryRenderer);
        
        // Table Playfield
        SDL_Rect tableRect = {0, 0, MAIN_WINDOW_WIDTH, MAIN_WINDOW_HEIGHT};
        if (tableVideoPlayer && tableVideoCtx.texture) {
            SDL_RenderCopy(primaryRenderer, tableVideoCtx.texture, nullptr, &tableRect);
        } else if (tableTexture) {
            SDL_RenderCopy(primaryRenderer, tableTexture, nullptr, &tableRect);
        }
        
        // Wheel
        if (wheelTexture) {
            SDL_Rect wheelRect;
            wheelRect.w = WHEEL_IMAGE_SIZE;
            wheelRect.h = WHEEL_IMAGE_SIZE;
            wheelRect.x = MAIN_WINDOW_WIDTH - WHEEL_IMAGE_SIZE - WHEEL_IMAGE_MARGIN;
            wheelRect.y = MAIN_WINDOW_HEIGHT - WHEEL_IMAGE_SIZE - WHEEL_IMAGE_MARGIN;
            SDL_RenderCopy(primaryRenderer, wheelTexture, nullptr, &wheelRect);
        }
        
        // Table Title
        if (tableNameTexture) {
            SDL_Rect backgroundRect = {tableNameRect.x - 5, tableNameRect.y - 5, tableNameRect.w + 10, tableNameRect.h + 10};
            SDL_SetRenderDrawColor(primaryRenderer, 0, 0, 0, 128);
            SDL_RenderFillRect(primaryRenderer, &backgroundRect);
            SDL_RenderCopy(primaryRenderer, tableNameTexture, nullptr, &tableNameRect);
        }
        
        SDL_RenderPresent(primaryRenderer);
        
        // ----------------------- Render Secondary Screen ------------------------
        SDL_SetRenderDrawColor(secondaryRenderer, 0, 0, 0, 255);
        SDL_RenderClear(secondaryRenderer);
        
        // Backglass
        SDL_Rect backglassRect = {0, 0, BACKGLASS_MEDIA_WIDTH, BACKGLASS_MEDIA_HEIGHT};
        if (backglassVideoPlayer && backglassVideoCtx.texture) {
            SDL_RenderCopy(secondaryRenderer, backglassVideoCtx.texture, nullptr, &backglassRect);
        } else if (backglassTexture) {
            SDL_RenderCopy(secondaryRenderer, backglassTexture, nullptr, &backglassRect);
        }

        // DMD
        SDL_Rect dmdRect = {0, BACKGLASS_MEDIA_HEIGHT, DMD_MEDIA_WIDTH, DMD_MEDIA_HEIGHT};
        if (dmdVideoPlayer && dmdVideoCtx.texture) {
            SDL_RenderCopy(secondaryRenderer, dmdVideoCtx.texture, nullptr, &dmdRect);
        } else if (dmdTexture) {
            SDL_RenderCopy(secondaryRenderer, dmdTexture, nullptr, &dmdRect);
        }
        
        SDL_RenderPresent(secondaryRenderer);
        
        // Delay to limit the frame rate to approximately 60 frames per second
        SDL_Delay(16);
    }

    // ----------------- Cleanup ------------------------

    cleanupVideoContext(tableVideoCtx, tableVideoPlayer);
    cleanupVideoContext(backglassVideoCtx, backglassVideoPlayer);
    cleanupVideoContext(dmdVideoCtx, dmdVideoPlayer);

    if (tableTexture)       SDL_DestroyTexture(tableTexture);
    if (wheelTexture)       SDL_DestroyTexture(wheelTexture);
    if (backglassTexture)   SDL_DestroyTexture(backglassTexture);
    if (dmdTexture)         SDL_DestroyTexture(dmdTexture);
    if (tableNameTexture)   SDL_DestroyTexture(tableNameTexture);

    if (font) TTF_CloseFont(font);

    if (tableChangeSound) Mix_FreeChunk(tableChangeSound);
    if (tableLoadSound)   Mix_FreeChunk(tableLoadSound);

    SDL_DestroyRenderer(secondaryRenderer);
    SDL_DestroyWindow(secondaryWindow);
    SDL_DestroyRenderer(primaryRenderer);
    SDL_DestroyWindow(primaryWindow);
    
    libvlc_release(vlcInstance);
    Mix_CloseAudio();
    TTF_Quit();
    IMG_Quit();
    SDL_Quit();
    
    return 0;
}