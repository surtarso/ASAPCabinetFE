#include <iostream>
#include <memory>
#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>
#include <SDL_mixer.h>
#include <vlc/vlc.h>
#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_sdlrenderer2.h"
#include "config_gui.h"
#include "config.h"
#include "video.h"
#include "table_utils.h"
#include "render_utils.h"
#include "sdl_guards.h"
#include "transition_manager.h"
#include "asset_manager.h"
#include "input_manager.h"
#include "screenshot_utils.h"

#define ASAPCAB_VERSION "2.0.0"
#define CHECK_SDL(x, msg) if (!(x)) { std::cerr << msg << ": " << SDL_GetError() << std::endl; return 1; }

void updateVideoTexture(VideoContext* video, SDL_Renderer* renderer) {
    (void)renderer;  // Silence warning
    if (video && video->texture && video->pixels && video->mutex && video->player) {
        if (SDL_LockMutex(video->mutex) == 0) {
            if (SDL_UpdateTexture(video->texture, nullptr, video->pixels, video->pitch) != 0) {
                std::cerr << "SDL_UpdateTexture failed: " << SDL_GetError() << std::endl;
            }
            SDL_UnlockMutex(video->mutex);
        } else {
            std::cerr << "SDL_LockMutex failed: " << SDL_GetError() << std::endl;
        }
    }
}

int main(int argc, char* argv[]) {
    // Check for --version flag
    if (argc == 2 && std::string(argv[1]) == "--version") {
        std::cout << "ASAPCabinetFE version " << ASAPCAB_VERSION << std::endl;
        return 0;
    }

    initialize_config("config.ini");

    SDLInitGuard sdlInit(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_AUDIO);
    CHECK_SDL(sdlInit.success, "SDL_Init failed");

    IMGInitGuard imgInit(IMG_INIT_PNG | IMG_INIT_JPG);
    CHECK_SDL(imgInit.flags, "IMG_Init failed");

    TTFInitGuard ttfInit;
    CHECK_SDL(ttfInit.success, "TTF_Init failed");

    MixerGuard mixerGuard(44100, MIX_DEFAULT_FORMAT, 2, 2048);
    CHECK_SDL(mixerGuard.success, "Mix_OpenAudio failed");

    auto primaryWindow = std::unique_ptr<SDL_Window, void(*)(SDL_Window*)>(
        SDL_CreateWindow("Playfield", SDL_WINDOWPOS_CENTERED_DISPLAY(MAIN_WINDOW_MONITOR), SDL_WINDOWPOS_CENTERED,
            MAIN_WINDOW_WIDTH, MAIN_WINDOW_HEIGHT, SDL_WINDOW_SHOWN | SDL_WINDOW_BORDERLESS),
        SDL_DestroyWindow);
    CHECK_SDL(primaryWindow, "Failed to create primary window");

    auto primaryRenderer = std::unique_ptr<SDL_Renderer, void(*)(SDL_Renderer*)>(
        SDL_CreateRenderer(primaryWindow.get(), -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC),
        SDL_DestroyRenderer);
    CHECK_SDL(primaryRenderer, "Failed to create primary renderer");

    auto secondaryWindow = std::unique_ptr<SDL_Window, void(*)(SDL_Window*)>(
        SDL_CreateWindow("Backglass", SDL_WINDOWPOS_CENTERED_DISPLAY(SECOND_WINDOW_MONITOR), SDL_WINDOWPOS_CENTERED,
            SECOND_WINDOW_WIDTH, SECOND_WINDOW_HEIGHT, SDL_WINDOW_SHOWN | SDL_WINDOW_BORDERLESS),
        SDL_DestroyWindow);
    CHECK_SDL(secondaryWindow, "Failed to create secondary window");

    auto secondaryRenderer = std::unique_ptr<SDL_Renderer, void(*)(SDL_Renderer*)>(
        SDL_CreateRenderer(secondaryWindow.get(), -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC),
        SDL_DestroyRenderer);
    CHECK_SDL(secondaryRenderer, "Failed to create secondary renderer");

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplSDL2_InitForSDLRenderer(primaryWindow.get(), primaryRenderer.get());
    ImGui_ImplSDLRenderer2_Init(primaryRenderer.get());

    bool showConfig = false;
    IniEditor configEditor("config.ini", showConfig);

    auto font = std::unique_ptr<TTF_Font, void(*)(TTF_Font*)>(TTF_OpenFont(FONT_PATH.c_str(), FONT_SIZE), TTF_CloseFont);
    if (!font) std::cerr << "Failed to load font: " << TTF_GetError() << std::endl;

    auto tableChangeSound = std::unique_ptr<Mix_Chunk, void(*)(Mix_Chunk*)>(Mix_LoadWAV(TABLE_CHANGE_SOUND.c_str()), Mix_FreeChunk);
    if (!tableChangeSound) std::cerr << "Mix_LoadWAV Error: " << Mix_GetError() << std::endl;

    auto tableLoadSound = std::unique_ptr<Mix_Chunk, void(*)(Mix_Chunk*)>(Mix_LoadWAV(TABLE_LOAD_SOUND.c_str()), Mix_FreeChunk);
    if (!tableLoadSound) std::cerr << "Mix_LoadWAV Error: " << Mix_GetError() << std::endl;

    std::vector<Table> tables = loadTableList();
    if (tables.empty()) { std::cerr << "Edit config.ini, no .vpx files found in " << VPX_TABLES_PATH << std::endl; return 1; }

    size_t currentIndex = 0;
    AssetManager assets(primaryRenderer.get(), secondaryRenderer.get(), font.get());
    TransitionManager transitionManager;
    InputManager inputManager;
    bool quit = false;
    SDL_Event event;

    assets.loadTableAssets(currentIndex, tables);

    while (!quit) {
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT) quit = true;
            if (event.type == SDL_KEYDOWN) {
                if (inputManager.isToggleConfig(event)) showConfig = !showConfig;

                if (showConfig) {
                    configEditor.handleEvent(event);
                } else {
                    if (inputManager.isPreviousTable(event)) {
                        size_t newIndex = (currentIndex + tables.size() - 1) % tables.size();
                        if (newIndex != currentIndex) {
                            transitionManager.startTransition(assets.getTableVideoPlayer(), assets.getBackglassVideoPlayer(), assets.getDmdVideoPlayer(), tableChangeSound.get());
                            currentIndex = newIndex;
                        }
                    }
                    else if (inputManager.isNextTable(event)) {
                        size_t newIndex = (currentIndex + 1) % tables.size();
                        if (newIndex != currentIndex) {
                            transitionManager.startTransition(assets.getTableVideoPlayer(), assets.getBackglassVideoPlayer(), assets.getDmdVideoPlayer(), tableChangeSound.get());
                            currentIndex = newIndex;
                        }
                    }
                    else if (inputManager.isFastPrevTable(event)) {
                        size_t newIndex = (currentIndex + tables.size() - 10) % tables.size();
                        if (newIndex != currentIndex) {
                            transitionManager.startTransition(assets.getTableVideoPlayer(), assets.getBackglassVideoPlayer(), assets.getDmdVideoPlayer(), tableChangeSound.get());
                            currentIndex = newIndex;
                        }
                    }
                    else if (inputManager.isFastNextTable(event)) {
                        size_t newIndex = (currentIndex + 10) % tables.size();
                        if (newIndex != currentIndex) {
                            transitionManager.startTransition(assets.getTableVideoPlayer(), assets.getBackglassVideoPlayer(), assets.getDmdVideoPlayer(), tableChangeSound.get());
                            currentIndex = newIndex;
                        }
                    }
                    else if (inputManager.isJumpNextLetter(event)) {
                        char currentLetter = toupper(tables[currentIndex].tableName[0]);
                        char nextLetter = currentLetter + 1;
                        bool found = false;
                        size_t newIndex = currentIndex;
                        for (; nextLetter <= 'Z'; ++nextLetter) {
                            if (letterIndex.find(nextLetter) != letterIndex.end()) {
                                newIndex = letterIndex[nextLetter];
                                found = true;
                                break;
                            }
                        }
                        if (!found) {
                            for (nextLetter = 'A'; nextLetter < currentLetter; ++nextLetter) {
                                if (letterIndex.find(nextLetter) != letterIndex.end()) {
                                    newIndex = letterIndex[nextLetter];
                                    found = true;
                                    break;
                                }
                            }
                        }
                        if (found && newIndex != currentIndex) {
                            transitionManager.startTransition(assets.getTableVideoPlayer(), assets.getBackglassVideoPlayer(), assets.getDmdVideoPlayer(), tableChangeSound.get());
                            currentIndex = newIndex;
                        }
                    }
                    else if (inputManager.isJumpPrevLetter(event)) {
                        char currentLetter = toupper(tables[currentIndex].tableName[0]);
                        char prevLetter = currentLetter - 1;
                        bool found = false;
                        size_t newIndex = currentIndex;
                        for (; prevLetter >= 'A'; --prevLetter) {
                            if (letterIndex.find(prevLetter) != letterIndex.end()) {
                                newIndex = letterIndex[prevLetter];
                                found = true;
                                break;
                            }
                        }
                        if (!found) {
                            for (prevLetter = 'Z'; prevLetter > currentLetter; --prevLetter) {
                                if (letterIndex.find(prevLetter) != letterIndex.end()) {
                                    newIndex = letterIndex[prevLetter];
                                    found = true;
                                    break;
                                }
                            }
                        }
                        if (found && newIndex != currentIndex) {
                            transitionManager.startTransition(assets.getTableVideoPlayer(), assets.getBackglassVideoPlayer(), assets.getDmdVideoPlayer(), tableChangeSound.get());
                            currentIndex = newIndex;
                        }
                    }
                    else if (inputManager.isLaunchTable(event)) {
                        if (tableLoadSound) Mix_PlayChannel(-1, tableLoadSound.get(), 0);
                        std::string command = VPX_START_ARGS + " " + VPX_EXECUTABLE_CMD + " " + VPX_SUB_CMD + " \"" + tables[currentIndex].vpxFile + "\" " + VPX_END_ARGS;
                        std::cout << "Launching: " << command << std::endl;
                        int result = std::system(command.c_str());
                        if (result != 0) {
                            std::cerr << "Warning: VPX launch failed with exit code " << result << std::endl;
                            // Continue running—failure isn’t fatal to frontend
                        }
                    }
                    else if (inputManager.isScreenshotMode(event)) {
                        std::cout << "Entering screenshot mode for: " << tables[currentIndex].vpxFile << std::endl;
                        launch_screenshot_mode(tables[currentIndex].vpxFile);
                    }
                    else if (inputManager.isQuit(event)) quit = true;
                }
            }
        }

        ImGui_ImplSDLRenderer2_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        VideoContext* tableVideo = assets.getTableVideoPlayer();
        VideoContext* backglassVideo = assets.getBackglassVideoPlayer();
        VideoContext* dmdVideo = assets.getDmdVideoPlayer();
        VideoContext* oldTableVideo = assets.getOldTableVideoPlayer();
        VideoContext* oldBackglassVideo = assets.getOldBackglassVideoPlayer();
        VideoContext* oldDmdVideo = assets.getOldDmdVideoPlayer();

        Uint32 now = SDL_GetTicks();
        transitionManager.updateTransition(now, assets);
        transitionManager.loadNewContent([&]() { assets.loadTableAssets(currentIndex, tables); });

        if (!transitionManager.isTransitionActive()) {
            updateVideoTexture(tableVideo, primaryRenderer.get());
            updateVideoTexture(backglassVideo, secondaryRenderer.get());
            updateVideoTexture(dmdVideo, secondaryRenderer.get());
        }

        SDL_SetRenderDrawColor(primaryRenderer.get(), 32, 32, 32, 255);
        SDL_RenderClear(primaryRenderer.get());

        SDL_Rect tableRect = {0, 0, MAIN_WINDOW_WIDTH, MAIN_WINDOW_HEIGHT};
        if (transitionManager.shouldMaskFrame()) {
            // Mask with black during the glitchy frame
            SDL_SetRenderDrawColor(primaryRenderer.get(), 0, 0, 0, 255);
            SDL_RenderClear(primaryRenderer.get());
        } else if (transitionManager.isTransitionActive() && oldTableVideo && oldTableVideo->texture) {
            SDL_RenderCopy(primaryRenderer.get(), oldTableVideo->texture, nullptr, &tableRect);
        } else if (!transitionManager.isTransitionActive() && tableVideo && tableVideo->texture) {
            SDL_RenderCopy(primaryRenderer.get(), tableVideo->texture, nullptr, &tableRect);
        } else if (!transitionManager.isTransitionActive() && assets.getTableTexture()) {
            SDL_RenderCopy(primaryRenderer.get(), assets.getTableTexture(), nullptr, &tableRect);
        }

        if (assets.getWheelTexture()) {
            SDL_Rect wheelRect = {MAIN_WINDOW_WIDTH - WHEEL_IMAGE_SIZE - WHEEL_IMAGE_MARGIN,
                                  MAIN_WINDOW_HEIGHT - WHEEL_IMAGE_SIZE - WHEEL_IMAGE_MARGIN,
                                  WHEEL_IMAGE_SIZE, WHEEL_IMAGE_SIZE};
            SDL_RenderCopy(primaryRenderer.get(), assets.getWheelTexture(), nullptr, &wheelRect);
        }

        if (assets.getTableNameTexture()) {
            SDL_Rect nameRect = assets.getTableNameRect();
            nameRect.x = 10;
            nameRect.y = MAIN_WINDOW_HEIGHT - nameRect.h - 10;
            SDL_Rect bgRect = {nameRect.x - 5, nameRect.y - 5, nameRect.w + 10, nameRect.h + 10};
            SDL_SetRenderDrawColor(primaryRenderer.get(), 0, 0, 0, 128);
            SDL_RenderFillRect(primaryRenderer.get(), &bgRect);
            SDL_RenderCopy(primaryRenderer.get(), assets.getTableNameTexture(), nullptr, &nameRect);
        }

        if (showConfig) configEditor.drawGUI();

        ImGui::Render();
        ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), primaryRenderer.get());
        SDL_RenderPresent(primaryRenderer.get());

        SDL_SetRenderDrawColor(secondaryRenderer.get(), 0, 0, 0, 255);
        SDL_RenderClear(secondaryRenderer.get());

        SDL_Rect backglassRect = {0, 0, BACKGLASS_MEDIA_WIDTH, BACKGLASS_MEDIA_HEIGHT};
        if (transitionManager.shouldMaskFrame()) {
            SDL_SetRenderDrawColor(secondaryRenderer.get(), 0, 0, 0, 255);
            SDL_RenderClear(secondaryRenderer.get());
        } else if (transitionManager.isTransitionActive() && oldBackglassVideo && oldBackglassVideo->texture) {
            SDL_RenderCopy(secondaryRenderer.get(), oldBackglassVideo->texture, nullptr, &backglassRect);
        } else if (!transitionManager.isTransitionActive() && backglassVideo && backglassVideo->texture) {
            SDL_RenderCopy(secondaryRenderer.get(), backglassVideo->texture, nullptr, &backglassRect);
        } else if (!transitionManager.isTransitionActive() && assets.getBackglassTexture()) {
            SDL_RenderCopy(secondaryRenderer.get(), assets.getBackglassTexture(), nullptr, &backglassRect);
        }

        SDL_Rect dmdRect = {0, BACKGLASS_MEDIA_HEIGHT, DMD_MEDIA_WIDTH, DMD_MEDIA_HEIGHT};
        if (transitionManager.shouldMaskFrame()) {
            SDL_SetRenderDrawColor(secondaryRenderer.get(), 0, 0, 0, 255);
            SDL_RenderClear(secondaryRenderer.get());
        } else if (transitionManager.isTransitionActive() && oldDmdVideo && oldDmdVideo->texture) {
            SDL_RenderCopy(secondaryRenderer.get(), oldDmdVideo->texture, nullptr, &dmdRect);
        } else if (!transitionManager.isTransitionActive() && dmdVideo && dmdVideo->texture) {
            SDL_RenderCopy(secondaryRenderer.get(), dmdVideo->texture, nullptr, &dmdRect);
        } else if (!transitionManager.isTransitionActive() && assets.getDmdTexture()) {
            SDL_RenderCopy(secondaryRenderer.get(), assets.getDmdTexture(), nullptr, &dmdRect);
        }

        SDL_RenderPresent(secondaryRenderer.get());

        if (!transitionManager.isTransitionActive()) {
            assets.clearOldVideoPlayers();
        }
    }

    cleanupVideoContext(assets.getTableVideoPlayer());
    cleanupVideoContext(assets.getBackglassVideoPlayer());
    cleanupVideoContext(assets.getDmdVideoPlayer());
    assets.clearOldVideoPlayers();

    ImGui_ImplSDLRenderer2_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    return 0;
}