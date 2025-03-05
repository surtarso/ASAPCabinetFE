// main.cpp
// ASAP-CABINET-FE in C++/SDL2
// - Scans VPX_TABLES_PATH recursively for .vpx files
// - For each table, loads filename. (videos)/images for the playfield, wheel, backglass and DMD
// - Creates two windows: primary (table name/wheel/table playfield)
//                    and secondary (backglass with dmd)
// - Uses left/right arrow/shift keys to change tables with a fade transition
// - Press Enter to launch the table via vpinballx_gl, 'q'/esc to quit
// Dependencies:
// sudo apt-get install libsdl2-dev libsdl2-image-dev libsdl2-ttf-dev libsdl2-mixer-dev libvlc-dev vlc
// Compile:
// g++ main.cpp -std=c++17 -I/usr/include/SDL2 -D_REENTRANT -lSDL2 -lSDL2_image -lSDL2_ttf -lSDL2_mixer -lvlc -o ASAPCabinetFE

#include <algorithm>
#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>
#include <SDL_mixer.h>
#include <vlc/vlc.h>
#include <iostream>
#include <filesystem>
#include <vector>
#include <string>
#include <cstdlib>

namespace fs = std::filesystem;

// ------------------ Configuration Constants ------------------

const std::string VPX_TABLES_PATH    = "/home/tarso/Games/vpinball/build/tables/";
const std::string VPX_EXECUTABLE_CMD = "/home/tarso/Games/vpinball/build/VPinballX_GL";
const std::string VPX_SUB_CMD        = "-Play";

const std::string DEFAULT_TABLE_IMAGE     = "img/default_table.png";
const std::string DEFAULT_BACKGLASS_IMAGE = "img/default_backglass.png";
const std::string DEFAULT_DMD_IMAGE       = "img/default_dmd.png";
const std::string DEFAULT_WHEEL_IMAGE     = "img/default_wheel.png";

const std::string CUSTOM_TABLE_IMAGE      = "images/table.png";
const std::string CUSTOM_BACKGLASS_IMAGE  = "images/backglass.png";
const std::string CUSTOM_DMD_IMAGE        = "images/marquee.png";
const std::string CUSTOM_WHEEL_IMAGE      = "images/wheel.png";

const std::string CUSTOM_TABLE_VIDEO      = "video/table.mp4";
const std::string CUSTOM_BACKGLASS_VIDEO  = "video/backglass.mp4";
const std::string CUSTOM_DMD_VIDEO        = "video/dmd.mp4";

const int MAIN_WINDOW_MONITOR      = 1;
const int MAIN_WINDOW_WIDTH        = 1080;
const int MAIN_WINDOW_HEIGHT       = 1920;
const int WHEEL_IMAGE_SIZE         = 300;
const int WHEEL_IMAGE_MARGIN       = 24;
const std::string FONT_PATH        = "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf";
const int FONT_SIZE                = 28;

const int BACKGLASS_WINDOW_MONITOR = 0;
const int BACKGLASS_WINDOW_WIDTH   = 1024;
const int BACKGLASS_WINDOW_HEIGHT  = 1024;
const int BACKGLASS_MEDIA_WIDTH    = 1024;
const int BACKGLASS_MEDIA_HEIGHT   = 768;
const int DMD_MEDIA_WIDTH          = 1024;
const int DMD_MEDIA_HEIGHT         = 256;

const int FADE_DURATION_MS         = 300;
const Uint8 FADE_TARGET_ALPHA      = 128;

const std::string TABLE_CHANGE_SOUND = "snd/table_change.mp3";
const std::string TABLE_LOAD_SOUND   = "snd/table_load.mp3";

// ------------------ Data Structures ------------------

struct Table {
    std::string tableName;
    std::string vpxFile;
    std::string folder;
    std::string tableImage;
    std::string wheelImage;
    std::string backglassImage;
    std::string dmdImage;
    std::string tableVideo;
    std::string backglassVideo;
    std::string dmdVideo;
};

struct VideoContext {
    SDL_Texture* texture;
    Uint8* pixels;
    int pitch;
    SDL_mutex* mutex;
    int width;
    int height;
    bool updated;
};

// ------------------ Utility Functions ------------------

std::string getImagePath(const std::string &root, const std::string &imagePath, const std::string &defaultPath) {
    fs::path imageFile = fs::path(root) / imagePath;
    if (fs::exists(imageFile))
        return imageFile.string();
    return defaultPath;
}

std::string getVideoPath(const std::string &root, const std::string &videoPath) {
    fs::path videoFile = fs::path(root) / videoPath;
    if (fs::exists(videoFile))
        return videoFile.string();
    return "";
}

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
            table.tableVideo     = getVideoPath(table.folder, CUSTOM_TABLE_VIDEO);
            table.backglassVideo = getVideoPath(table.folder, CUSTOM_BACKGLASS_VIDEO);
            table.dmdVideo       = getVideoPath(table.folder, CUSTOM_DMD_VIDEO);
            tables.push_back(table);
        }
    }
    std::sort(tables.begin(), tables.end(), [](const Table &a, const Table &b) {
        return a.tableName < b.tableName;
    });
    return tables;
}

SDL_Texture* loadTexture(SDL_Renderer* renderer, const std::string &path, const std::string &fallbackPath) {
    SDL_Texture* tex = IMG_LoadTexture(renderer, path.c_str());
    if (!tex) {
        std::cerr << "Failed to load " << path << ". Using fallback." << std::endl;
        tex = IMG_LoadTexture(renderer, fallbackPath.c_str());
    }
    return tex;
}

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

void* lock(void* data, void** pixels) {
    VideoContext* ctx = static_cast<VideoContext*>(data);
    if (!ctx || !ctx->mutex) return nullptr;
    SDL_LockMutex(ctx->mutex);
    *pixels = ctx->pixels;
    return nullptr;
}

void unlock(void* data, void* id, void* const* pixels) {
    VideoContext* ctx = static_cast<VideoContext*>(data);
    if (ctx) {
        ctx->updated = true;
        SDL_UnlockMutex(ctx->mutex);
    }
}

void display(void* data, void* id) {}

void onMediaPlayerEndReached(const libvlc_event_t* event, void* data) {
    libvlc_media_player_t* player = static_cast<libvlc_media_player_t*>(data);
    if (player) libvlc_media_player_set_position(player, 0);
}

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

    // Use BGRA instead of RGBA to match common VLC output
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

    ctx.pitch = width * 4;
    ctx.mutex = SDL_CreateMutex();
    ctx.width = width;
    ctx.height = height;
    ctx.updated = false;

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

void launchTable(const Table &table) {
    std::string command = VPX_EXECUTABLE_CMD + " " + VPX_SUB_CMD + " \"" + table.vpxFile + "\"";
    std::cout << "Launching: " << command << std::endl;
    std::system(command.c_str());
}

// ------------------ Main Application ------------------

enum class TransitionState { IDLE, FADING_OUT, FADING_IN };

int main(int argc, char* argv[]) {
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

    libvlc_instance_t* vlcInstance = libvlc_new(0, nullptr);
    if (!vlcInstance) {
        std::cerr << "Failed to initialize VLC instance." << std::endl;
        Mix_CloseAudio();
        TTF_Quit();
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

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
    
    SDL_Window* secondaryWindow = SDL_CreateWindow("Backglass",
        SDL_WINDOWPOS_CENTERED_DISPLAY(BACKGLASS_WINDOW_MONITOR), SDL_WINDOWPOS_CENTERED,
        BACKGLASS_WINDOW_WIDTH, BACKGLASS_WINDOW_HEIGHT, SDL_WINDOW_SHOWN | SDL_WINDOW_BORDERLESS);
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
    
    TTF_Font* font = TTF_OpenFont(FONT_PATH.c_str(), FONT_SIZE);
    if (!font) {
        std::cerr << "Failed to load font: " << TTF_GetError() << std::endl;
    }
    
    Mix_Chunk* tableChangeSound = Mix_LoadWAV(TABLE_CHANGE_SOUND.c_str());
    if (!tableChangeSound) {
        std::cerr << "Mix_LoadWAV Error: " << Mix_GetError() << std::endl;
    }
    Mix_Chunk* tableLoadSound = Mix_LoadWAV(TABLE_LOAD_SOUND.c_str());
    if (!tableLoadSound) {
        std::cerr << "Mix_LoadWAV Error: " << Mix_GetError() << std::endl;
    }

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
    
    size_t currentIndex = 0;
    SDL_Texture* tableTexture = nullptr;
    SDL_Texture* wheelTexture = nullptr;
    SDL_Texture* backglassTexture = nullptr;
    SDL_Texture* dmdTexture = nullptr;
    SDL_Texture* tableNameTexture = nullptr;
    SDL_Rect tableNameRect = {0, 0, 0, 0};

    libvlc_media_player_t* tableVideoPlayer = nullptr;
    libvlc_media_player_t* backglassVideoPlayer = nullptr;
    libvlc_media_player_t* dmdVideoPlayer = nullptr;
    VideoContext tableVideoCtx = {nullptr, nullptr, 0, nullptr, 0, 0, false};
    VideoContext backglassVideoCtx = {nullptr, nullptr, 0, nullptr, 0, 0, false};
    VideoContext dmdVideoCtx = {nullptr, nullptr, 0, nullptr, 0, 0, false};

    auto loadCurrentTableTextures = [&]() {
        cleanupVideoContext(tableVideoCtx, tableVideoPlayer);
        cleanupVideoContext(backglassVideoCtx, backglassVideoPlayer);
        cleanupVideoContext(dmdVideoCtx, dmdVideoPlayer);

        if (tableTexture) { SDL_DestroyTexture(tableTexture); tableTexture = nullptr; }
        if (backglassTexture) { SDL_DestroyTexture(backglassTexture); backglassTexture = nullptr; }
        if (dmdTexture) { SDL_DestroyTexture(dmdTexture); dmdTexture = nullptr; }
        if (tableNameTexture) { SDL_DestroyTexture(tableNameTexture); tableNameTexture = nullptr; }

        const Table &tbl = tables[currentIndex];

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

        wheelTexture = loadTexture(primaryRenderer, tbl.wheelImage, DEFAULT_WHEEL_IMAGE);

        if (font) {
            SDL_Color textColor = {255, 255, 255, 255};
            tableNameTexture = renderText(primaryRenderer, font, tbl.tableName, textColor, tableNameRect);
            tableNameRect.x = 10;
            tableNameRect.y = MAIN_WINDOW_HEIGHT - tableNameRect.h - 20;
        }
    };

    loadCurrentTableTextures();

    TransitionState transitionState = TransitionState::IDLE;
    Uint32 transitionStartTime = 0;
    bool quit = false;
    SDL_Event event;

    while (!quit) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                quit = true;
            }
            else if (event.type == SDL_KEYDOWN && transitionState == TransitionState::IDLE) {
                if (event.key.keysym.sym == SDLK_LEFT || event.key.keysym.sym == SDLK_LSHIFT) {
                    if (tableVideoPlayer) libvlc_media_player_stop(tableVideoPlayer);
                    if (backglassVideoPlayer) libvlc_media_player_stop(backglassVideoPlayer);
                    if (dmdVideoPlayer) libvlc_media_player_stop(dmdVideoPlayer);
                    if (tableChangeSound) Mix_PlayChannel(-1, tableChangeSound, 0);
                    currentIndex = (currentIndex + tables.size() - 1) % tables.size();
                    transitionState = TransitionState::FADING_OUT;
                    transitionStartTime = SDL_GetTicks();
                }
                else if (event.key.keysym.sym == SDLK_RIGHT || event.key.keysym.sym == SDLK_RSHIFT) {
                    if (tableVideoPlayer) libvlc_media_player_stop(tableVideoPlayer);
                    if (backglassVideoPlayer) libvlc_media_player_stop(backglassVideoPlayer);
                    if (dmdVideoPlayer) libvlc_media_player_stop(dmdVideoPlayer);
                    if (tableChangeSound) Mix_PlayChannel(-1, tableChangeSound, 0);
                    currentIndex = (currentIndex + 1) % tables.size();
                    transitionState = TransitionState::FADING_OUT;
                    transitionStartTime = SDL_GetTicks();
                }
                else if (event.key.keysym.sym == SDLK_RETURN || event.key.keysym.sym == SDLK_KP_ENTER) {
                    if (tableLoadSound) Mix_PlayChannel(-1, tableLoadSound, 0);
                    launchTable(tables[currentIndex]);
                }
                else if (event.key.keysym.sym == SDLK_ESCAPE || event.key.keysym.sym == SDLK_q) {
                    quit = true;
                }
            }
        }

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

        if (tableTexture) SDL_SetTextureAlphaMod(tableTexture, currentAlpha);
        if (wheelTexture) SDL_SetTextureAlphaMod(wheelTexture, currentAlpha);
        if (backglassTexture) SDL_SetTextureAlphaMod(backglassTexture, currentAlpha);
        if (dmdTexture) SDL_SetTextureAlphaMod(dmdTexture, currentAlpha);
        if (tableNameTexture) SDL_SetTextureAlphaMod(tableNameTexture, currentAlpha);
        if (tableVideoCtx.texture) SDL_SetTextureAlphaMod(tableVideoCtx.texture, currentAlpha);
        if (backglassVideoCtx.texture) SDL_SetTextureAlphaMod(backglassVideoCtx.texture, currentAlpha);
        if (dmdVideoCtx.texture) SDL_SetTextureAlphaMod(dmdVideoCtx.texture, currentAlpha);

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

        SDL_SetRenderDrawColor(primaryRenderer, 32, 32, 32, 255);
        SDL_RenderClear(primaryRenderer);
        
        SDL_Rect tableRect = {0, 0, MAIN_WINDOW_WIDTH, MAIN_WINDOW_HEIGHT};
        if (tableVideoPlayer && tableVideoCtx.texture) {
            SDL_RenderCopy(primaryRenderer, tableVideoCtx.texture, nullptr, &tableRect);
        } else if (tableTexture) {
            SDL_RenderCopy(primaryRenderer, tableTexture, nullptr, &tableRect);
        }
        
        if (wheelTexture) {
            SDL_Rect wheelRect;
            wheelRect.w = WHEEL_IMAGE_SIZE;
            wheelRect.h = WHEEL_IMAGE_SIZE;
            wheelRect.x = MAIN_WINDOW_WIDTH - WHEEL_IMAGE_SIZE - WHEEL_IMAGE_MARGIN;
            wheelRect.y = MAIN_WINDOW_HEIGHT - WHEEL_IMAGE_SIZE - WHEEL_IMAGE_MARGIN;
            SDL_RenderCopy(primaryRenderer, wheelTexture, nullptr, &wheelRect);
        }
        
        if (tableNameTexture) {
            SDL_Rect backgroundRect = {tableNameRect.x - 5, tableNameRect.y - 5, tableNameRect.w + 10, tableNameRect.h + 10};
            SDL_SetRenderDrawColor(primaryRenderer, 0, 0, 0, 128);
            SDL_RenderFillRect(primaryRenderer, &backgroundRect);
            SDL_RenderCopy(primaryRenderer, tableNameTexture, nullptr, &tableNameRect);
        }
        
        SDL_RenderPresent(primaryRenderer);
        
        SDL_SetRenderDrawColor(secondaryRenderer, 0, 0, 0, 255);
        SDL_RenderClear(secondaryRenderer);
        
        SDL_Rect backglassRect = {0, 0, BACKGLASS_MEDIA_WIDTH, BACKGLASS_MEDIA_HEIGHT};
        if (backglassVideoPlayer && backglassVideoCtx.texture) {
            SDL_RenderCopy(secondaryRenderer, backglassVideoCtx.texture, nullptr, &backglassRect);
        } else if (backglassTexture) {
            SDL_RenderCopy(secondaryRenderer, backglassTexture, nullptr, &backglassRect);
        }

        SDL_Rect dmdRect = {0, BACKGLASS_MEDIA_HEIGHT, DMD_MEDIA_WIDTH, DMD_MEDIA_HEIGHT};
        if (dmdVideoPlayer && dmdVideoCtx.texture) {
            SDL_RenderCopy(secondaryRenderer, dmdVideoCtx.texture, nullptr, &dmdRect);
        } else if (dmdTexture) {
            SDL_RenderCopy(secondaryRenderer, dmdTexture, nullptr, &dmdRect);
        }
        
        SDL_RenderPresent(secondaryRenderer);
        
        SDL_Delay(16);
    }

    cleanupVideoContext(tableVideoCtx, tableVideoPlayer);
    cleanupVideoContext(backglassVideoCtx, backglassVideoPlayer);
    cleanupVideoContext(dmdVideoCtx, dmdVideoPlayer);

    if (tableTexture) SDL_DestroyTexture(tableTexture);
    if (wheelTexture) SDL_DestroyTexture(wheelTexture);
    if (backglassTexture) SDL_DestroyTexture(backglassTexture);
    if (dmdTexture) SDL_DestroyTexture(dmdTexture);
    if (tableNameTexture) SDL_DestroyTexture(tableNameTexture);

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
    
    return 0;
}