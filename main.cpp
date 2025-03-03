// main.cpp
// ASAP-CABINET-FE in C++/SDL2
// - Scans VPX_TABLES_PATH recursively for .vpx files
// - For each table, loads images for the playfield, wheel, backglass and DMD
// - Creates two windows: primary (playfield) and secondary (backglass)
// - Uses left/right arrow keys to change tables with a fade transition
// - Press Enter to launch the table via an external process
// Required libraries: SDL2, SDL2_image, SDL2_ttf, SDL2_mixer (make sure these are installed)

// Dependencies:
// sudo apt-get install libsdl2-dev libsdl2-image-dev libsdl2-ttf-dev libsdl2-mixer-dev
// Compile:
// g++ main.cpp -std=c++17 -I/usr/include/SDL2 -D_REENTRANT -lSDL2 -lSDL2_image -lSDL2_ttf -lSDL2_mixer -o ASAPCabinetFE

#include <algorithm>
#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>
#include <SDL_mixer.h>
#include <iostream>
#include <filesystem>
#include <vector>
#include <string>
#include <cstdlib>   // for system()

namespace fs = std::filesystem;

// ------------------ Configuration Constants ------------------

// VPX table and executable paths
const std::string VPX_TABLES_PATH         = "/home/tarso/Games/vpinball/build/tables/";
const std::string VPX_EXECUTABLE_CMD      = "/home/tarso/Games/vpinball/build/VPinballX_GL";
const std::string VPX_SUB_CMD             = "-Play";

// Default image paths
const std::string DEFAULT_TABLE_IMAGE     = "img/default_table.png";
const std::string DEFAULT_BACKGLASS_IMAGE = "img/default_backglass.png";
const std::string DEFAULT_DMD_IMAGE       = "img/default_dmd.png";
const std::string DEFAULT_WHEEL_IMAGE     = "img/default_wheel.png";

// Per-table relative paths (inside each table’s folder)
const std::string CUSTOM_TABLE_IMAGE      = "images/table.png";
const std::string CUSTOM_BACKGLASS_IMAGE  = "images/backglass.png";
const std::string CUSTOM_MARQUEE_IMAGE    = "images/marquee.png";
const std::string CUSTOM_WHEEL_IMAGE      = "images/wheel.png";

// Main window dimensions and UI constants
const int MAIN_WINDOW_MONITOR            = 1;
const int MAIN_WINDOW_WIDTH              = 1080;
const int MAIN_WINDOW_HEIGHT             = 1920;
const int WHEEL_IMAGE_SIZE               = 300;
const int WHEEL_IMAGE_MARGIN             = 24;
const std::string FONT_PATH              = "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf";
const int FONT_SIZE                      = 28;

// Secondary window dimensions
const int BACKGLASS_WINDOW_MONITOR       = 0;
const int BACKGLASS_WINDOW_WIDTH         = 1024;
const int BACKGLASS_WINDOW_HEIGHT        = 1024;
const int BACKGLASS_MEDIA_WIDTH          = 1024;
const int BACKGLASS_MEDIA_HEIGHT         = 768;
const int MARQUEE_MEDIA_WIDTH            = 1024;
const int MARQUEE_MEDIA_HEIGHT           = 256;

// Fade transition settings
const int FADE_DURATION_MS               = 300;   // total duration for fade-out/in phases
const Uint8 FADE_TARGET_ALPHA            = 128;   // fade to 50% (128 out of 255)

// Sound file path
const std::string TABLE_CHANGE_SOUND_PATH = "snd/table_change.mp3";
const std::string TABLE_LOAD_SOUND_PATH = "snd/table_load.mp3";

// ------------------ Data Structures ------------------

struct Table {
    std::string tableName;        // Name of the table (derived from .vpx file name)
    std::string vpxFile;          // Path to the .vpx file
    std::string folder;           // Folder containing the .vpx file
    std::string tableImage;       // Path to the table image
    std::string wheelImage;       // Path to the wheel image
    std::string backglassImage;   // Path to the backglass image
    std::string marqueeImage;     // Path to the marquee image (defaults to DMD image)
};

// ------------------ Utility Functions ------------------

// Returns path if it exists, otherwise returns defaultPath.
std::string getImagePath(const std::string &root, const std::string &subpath, const std::string &defaultPath) {
    fs::path p = fs::path(root) / subpath;
    if (fs::exists(p))
        return p.string();
    else
        return defaultPath;
}

// Scans VPX_TABLES_PATH recursively for .vpx files and builds a vector of Table objects.
std::vector<Table> loadTableList() {
    std::vector<Table> tables;
    for (const auto &entry : fs::recursive_directory_iterator(VPX_TABLES_PATH)) {
        if (entry.is_regular_file() && entry.path().extension() == ".vpx") {
            Table table;
            table.vpxFile = entry.path().string();
            table.folder = entry.path().parent_path().string();
            table.tableName = entry.path().stem().string();
            // Build per-table image paths with fallbacks:
            table.wheelImage = getImagePath(table.folder, CUSTOM_WHEEL_IMAGE, DEFAULT_WHEEL_IMAGE);
            table.tableImage = getImagePath(table.folder, CUSTOM_TABLE_IMAGE, DEFAULT_TABLE_IMAGE);
            table.backglassImage = getImagePath(table.folder, CUSTOM_BACKGLASS_IMAGE, DEFAULT_BACKGLASS_IMAGE);
            table.marqueeImage = getImagePath(table.folder, CUSTOM_MARQUEE_IMAGE, DEFAULT_DMD_IMAGE);
            tables.push_back(table);
        }
    }
    // Sort alphabetically by table name
    std::sort(tables.begin(), tables.end(), [](const Table &a, const Table &b) {
        return a.tableName < b.tableName;
    });
    return tables;
}

// Loads an SDL_Texture from a file; if fails, loads the texture from fallbackPath.
SDL_Texture* loadTexture(SDL_Renderer* renderer, const std::string &path, const std::string &fallbackPath) {
    SDL_Texture* tex = IMG_LoadTexture(renderer, path.c_str());
    if (!tex) {
        std::cerr << "Failed to load " << path << ". Using fallback." << std::endl;
        tex = IMG_LoadTexture(renderer, fallbackPath.c_str());
    }
    return tex;
}

// Renders text using SDL_ttf and returns an SDL_Texture pointer.
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

// Launches the table using system() call.
void launchTable(const Table &table) {
    std::string command = VPX_EXECUTABLE_CMD + " " + VPX_SUB_CMD + " \"" + table.vpxFile + "\"";
    std::cout << "Launching: " << command << std::endl;
    std::system(command.c_str());
}

// ------------------ Main Application ------------------

enum class TransitionState { IDLE, FADING_OUT, FADING_IN };

int main(int argc, char* argv[]) {
    // Initialize SDL, SDL_image, and SDL_ttf
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_AUDIO) != 0) {
        std::cerr << "SDL_Init Error: " << SDL_GetError() << std::endl;
        return 1;
    }
    if (!(IMG_Init(IMG_INIT_PNG | IMG_INIT_JPG) & (IMG_INIT_PNG | IMG_INIT_JPG))) {
        std::cerr << "IMG_Init Error: " << IMG_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }
    if (TTF_Init() != 0) {
        std::cerr << "TTF_Init Error: " << TTF_GetError() << std::endl;
        IMG_Quit();
        SDL_Quit();
        return 1;
    }
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
        std::cerr << "SDL_mixer Error: " << Mix_GetError() << std::endl;
        return 1;
    }
    
    // Create primary window (playfield)
    SDL_Window* primaryWindow = SDL_CreateWindow("Playfield",
        SDL_WINDOWPOS_CENTERED_DISPLAY(MAIN_WINDOW_MONITOR), SDL_WINDOWPOS_CENTERED,
        MAIN_WINDOW_WIDTH, MAIN_WINDOW_HEIGHT, SDL_WINDOW_SHOWN | SDL_WINDOW_BORDERLESS);
    if (!primaryWindow) {
        std::cerr << "Failed to create primary window: " << SDL_GetError() << std::endl;
        return 1;
    }
    
    SDL_Renderer* primaryRenderer = SDL_CreateRenderer(primaryWindow, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!primaryRenderer) {
        std::cerr << "Failed to create primary renderer: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(primaryWindow);
        return 1;
    }
    
    // Create secondary window (backglass)
    SDL_Window* secondaryWindow = SDL_CreateWindow("Backglass",
        SDL_WINDOWPOS_CENTERED_DISPLAY(BACKGLASS_WINDOW_MONITOR), SDL_WINDOWPOS_CENTERED,
        BACKGLASS_WINDOW_WIDTH, BACKGLASS_WINDOW_HEIGHT, SDL_WINDOW_SHOWN | SDL_WINDOW_BORDERLESS);
    if (!secondaryWindow) {
        std::cerr << "Failed to create secondary window: " << SDL_GetError() << std::endl;
        SDL_DestroyRenderer(primaryRenderer);
        SDL_DestroyWindow(primaryWindow);
        return 1;
    }
    
    SDL_Renderer* secondaryRenderer = SDL_CreateRenderer(secondaryWindow, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!secondaryRenderer) {
        std::cerr << "Failed to create secondary renderer: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(secondaryWindow);
        SDL_DestroyRenderer(primaryRenderer);
        SDL_DestroyWindow(primaryWindow);
        return 1;
    }
    
    // Load font for table name display
    TTF_Font* font = TTF_OpenFont(FONT_PATH.c_str(), FONT_SIZE);
    if (!font) {
        std::cerr << "Failed to load font: " << TTF_GetError() << std::endl;
        // Continue without text if necessary
    }
    
    // Load sound effect
    Mix_Chunk* tableChangeSound = Mix_LoadWAV(TABLE_CHANGE_SOUND_PATH.c_str());
    if (!tableChangeSound) {
        std::cerr << "Mix_LoadWAV Error: " << Mix_GetError() << std::endl;
        return 1;
    }
    Mix_Chunk* tableLoadSound = Mix_LoadWAV(TABLE_LOAD_SOUND_PATH.c_str());
    if (!tableLoadSound) {
        std::cerr << "Mix_LoadWAV Error: " << Mix_GetError() << std::endl;
        return 1;
    }

    // Load table list from VPX_TABLES_PATH
    std::vector<Table> tables = loadTableList();
    if (tables.empty()) {
        std::cerr << "No .vpx files found in " << VPX_TABLES_PATH << std::endl;
        // Clean up and quit
        if (font) TTF_CloseFont(font);
        SDL_DestroyRenderer(secondaryRenderer);
        SDL_DestroyWindow(secondaryWindow);
        SDL_DestroyRenderer(primaryRenderer);
        SDL_DestroyWindow(primaryWindow);
        TTF_Quit();
        IMG_Quit();
        SDL_Quit();
        return 1;
    }
    
    // Current table index and textures
    size_t currentIndex = 0;
    SDL_Texture* tableTexture = nullptr;
    SDL_Texture* wheelTexture = nullptr;
    SDL_Texture* backglassTexture = nullptr;
    SDL_Texture* marqueeTexture = nullptr;
    SDL_Texture* tableNameTexture = nullptr;
    SDL_Rect tableNameRect = {0, 0, 0, 0};
    
    auto loadCurrentTableTextures = [&]() {
        // Clean up old textures if any
        if (tableTexture) { SDL_DestroyTexture(tableTexture); tableTexture = nullptr; }
        if (wheelTexture) { SDL_DestroyTexture(wheelTexture); wheelTexture = nullptr; }
        if (backglassTexture) { SDL_DestroyTexture(backglassTexture); backglassTexture = nullptr; }
        if (marqueeTexture) { SDL_DestroyTexture(marqueeTexture); marqueeTexture = nullptr; }
        if (tableNameTexture) { SDL_DestroyTexture(tableNameTexture); tableNameTexture = nullptr; }
        
        const Table &tbl = tables[currentIndex];
        tableTexture = loadTexture(primaryRenderer, tbl.tableImage, DEFAULT_TABLE_IMAGE);
        wheelTexture = loadTexture(primaryRenderer, tbl.wheelImage, DEFAULT_WHEEL_IMAGE);
        backglassTexture = loadTexture(secondaryRenderer, tbl.backglassImage, DEFAULT_BACKGLASS_IMAGE);
        marqueeTexture = loadTexture(secondaryRenderer, tbl.marqueeImage, DEFAULT_DMD_IMAGE);
        
        // Render table name text (if font loaded)
        if (font) {
            SDL_Color textColor = {255, 255, 255, 255}; // white
            tableNameTexture = renderText(primaryRenderer, font, tbl.tableName, textColor, tableNameRect);
            // Position the text at the bottom center with some padding.
            tableNameRect.x = 10;
            tableNameRect.y = MAIN_WINDOW_HEIGHT - tableNameRect.h - 20;
        }
    };
    
    loadCurrentTableTextures();
    
    // Transition state for fade animation
    TransitionState transitionState = TransitionState::IDLE;
    Uint32 transitionStartTime = 0;
    
    bool quit = false;
    SDL_Event event;
    
    // Main loop
    while (!quit) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                quit = true;
            }
            else if (event.type == SDL_KEYDOWN && transitionState == TransitionState::IDLE) {
                if (event.key.keysym.sym == SDLK_LEFT || event.key.keysym.sym == SDLK_LSHIFT) {
                    // Navigate LEFT
                    Mix_PlayChannel(-1, tableChangeSound, 0);
                    currentIndex = (currentIndex + tables.size() - 1) % tables.size();
                    transitionState = TransitionState::FADING_OUT;
                    transitionStartTime = SDL_GetTicks(); // LEFT arrow/shift
                }
                else if (event.key.keysym.sym == SDLK_RIGHT || event.key.keysym.sym == SDLK_RSHIFT) {
                    // Navigate RIGHT
                    Mix_PlayChannel(-1, tableChangeSound, 0);
                    currentIndex = (currentIndex + 1) % tables.size();
                    transitionState = TransitionState::FADING_OUT;
                    transitionStartTime = SDL_GetTicks(); // RIGHT arrow/shift
                }
                else if (event.key.keysym.sym == SDLK_RETURN || event.key.keysym.sym == SDLK_KP_ENTER) {
                    // LAUNCH the current table
                    Mix_PlayChannel(-1, tableLoadSound, 0);
                    launchTable(tables[currentIndex]); // ENTER to launch
                }
                else if (event.key.keysym.sym == SDLK_ESCAPE || event.key.keysym.sym == SDLK_q) {
                    quit = true; // ESC or ´q' to quit
                }
            }
        }
        
        // Determine current alpha modulation for fade transition
        Uint8 currentAlpha = 255;
        Uint32 now = SDL_GetTicks();
        if (transitionState != TransitionState::IDLE) {
            Uint32 elapsed = now - transitionStartTime;
            int halfDuration = FADE_DURATION_MS / 2;
            if (transitionState == TransitionState::FADING_OUT) {
                if (elapsed < (Uint32)halfDuration) {
                    // Linearly reduce alpha from 255 to FADE_TARGET_ALPHA
                    currentAlpha = 255 - (Uint8)(((255 - FADE_TARGET_ALPHA) * elapsed) / halfDuration);
                } else {
                    // Fade-out complete; update textures and begin fade-in phase.
                    loadCurrentTableTextures();
                    transitionState = TransitionState::FADING_IN;
                    transitionStartTime = SDL_GetTicks();
                    currentAlpha = FADE_TARGET_ALPHA;
                }
            }
            else if (transitionState == TransitionState::FADING_IN) {
                if (elapsed < (Uint32)halfDuration) {
                    // Increase alpha from FADE_TARGET_ALPHA to 255
                    currentAlpha = FADE_TARGET_ALPHA + (Uint8)(((255 - FADE_TARGET_ALPHA) * elapsed) / halfDuration);
                } else {
                    currentAlpha = 255;
                    transitionState = TransitionState::IDLE;
                }
            }
        }
        
        // Set alpha modulation for textures
        if (tableTexture) SDL_SetTextureAlphaMod(tableTexture, currentAlpha);
        if (wheelTexture) SDL_SetTextureAlphaMod(wheelTexture, currentAlpha);
        if (backglassTexture) SDL_SetTextureAlphaMod(backglassTexture, currentAlpha);
        if (marqueeTexture) SDL_SetTextureAlphaMod(marqueeTexture, currentAlpha);
        if (tableNameTexture) SDL_SetTextureAlphaMod(tableNameTexture, currentAlpha);
        
        // ---------------- Render Primary Window ----------------
        SDL_SetRenderDrawColor(primaryRenderer, 32, 32, 32, 255); // BG_COLOR (#202020)
        SDL_RenderClear(primaryRenderer);
        
        // Render table (playfield) image to fill the window.
        if (tableTexture) {
            SDL_Rect tableRect = {0, 0, MAIN_WINDOW_WIDTH, MAIN_WINDOW_HEIGHT};
            SDL_RenderCopy(primaryRenderer, tableTexture, nullptr, &tableRect);
        }
        
        // Render wheel image at bottom-right corner.
        if (wheelTexture) {
            SDL_Rect wheelRect;
            wheelRect.w = WHEEL_IMAGE_SIZE;
            wheelRect.h = WHEEL_IMAGE_SIZE;
            wheelRect.x = MAIN_WINDOW_WIDTH - WHEEL_IMAGE_SIZE - WHEEL_IMAGE_MARGIN;
            wheelRect.y = MAIN_WINDOW_HEIGHT - WHEEL_IMAGE_SIZE - WHEEL_IMAGE_MARGIN;
            SDL_RenderCopy(primaryRenderer, wheelTexture, nullptr, &wheelRect);
        }
        
        // Render table name text if available.
        if (tableNameTexture) {
            // Create a black background rectangle that matches the text size
            SDL_Rect backgroundRect = {tableNameRect.x - 5, tableNameRect.y - 5, tableNameRect.w + 10, tableNameRect.h + 10}; // Add padding

            // Set the background rectangle's color and alpha
            SDL_SetRenderDrawColor(primaryRenderer, 0, 0, 0, 128); // Black, alpha = 128 (semi-transparent)

            // Render the background rectangle
            SDL_RenderFillRect(primaryRenderer, &backgroundRect);

            // Render the text on top of the background
            SDL_RenderCopy(primaryRenderer, tableNameTexture, nullptr, &tableNameRect);
        }
        
        SDL_RenderPresent(primaryRenderer);
        
        // ---------------- Render Secondary Window ----------------
        SDL_SetRenderDrawColor(secondaryRenderer, 0, 0, 0, 255); // black background
        SDL_RenderClear(secondaryRenderer);
        
        // Render backglass image at top.
        if (backglassTexture) {
            SDL_Rect backglassRect = {0, 0, BACKGLASS_MEDIA_WIDTH, BACKGLASS_MEDIA_HEIGHT};
            SDL_RenderCopy(secondaryRenderer, backglassTexture, nullptr, &backglassRect);
        }
        // Render Marquee image at bottom (positioned below backglass)
        if (marqueeTexture) {
            SDL_Rect marqueeRect = {0, BACKGLASS_MEDIA_HEIGHT, MARQUEE_MEDIA_WIDTH, MARQUEE_MEDIA_HEIGHT};
            SDL_RenderCopy(secondaryRenderer, marqueeTexture, nullptr, &marqueeRect);
        }
        
        SDL_RenderPresent(secondaryRenderer);
        
        // Delay a short time to cap the frame rate (e.g., ~60 FPS)
        SDL_Delay(16);
    }
    
    // ---------------- Cleanup ----------------
    if (tableTexture) SDL_DestroyTexture(tableTexture);
    if (wheelTexture) SDL_DestroyTexture(wheelTexture);
    if (backglassTexture) SDL_DestroyTexture(backglassTexture);
    if (marqueeTexture) SDL_DestroyTexture(marqueeTexture);
    if (tableNameTexture) SDL_DestroyTexture(tableNameTexture);
    
    if (font) TTF_CloseFont(font);
    SDL_DestroyRenderer(primaryRenderer);
    SDL_DestroyWindow(primaryWindow);
    SDL_DestroyRenderer(secondaryRenderer);
    SDL_DestroyWindow(secondaryWindow);
    
    TTF_Quit();
    IMG_Quit();
    SDL_Quit();
    
    return 0;
}
