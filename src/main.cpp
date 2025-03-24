/*
 * main.cpp - Main entry point for ASAPCabinetFE.
 *
 * ASAPCabinetFE is a frontend for virtual pinball cabinets, designed to manage and display
 * Visual Pinball X (VPX) tables with support for dual-monitor setups (playfield and backglass/DMD).
 * It provides a user-friendly interface for navigating tables, configuring settings, playing videos,
 * and capturing screenshots, with smooth transitions and audio feedback.
 *
 * This file initializes SDL, ImGui, and other subsystems, sets up the main windows and renderers,
 * loads table assets, and runs the main loop to handle user input, rendering, and transitions.
 * Configuration changes are managed through a dedicated ConfigManager, which updates the application
 * state dynamically when settings are modified via the GUI.
 *
 * Tarso Galvão Mar/2025
 */

 #include <iostream>             // For std::cout, std::cerr (console output).
 #include <memory>               // For std::unique_ptr (smart pointers).
 #include <filesystem>           // For std::filesystem (path checking).
 #include <unistd.h>             // For readlink (symbolic link resolution).
 #include <limits.h>             // For PATH_MAX (path length limits).
 #include <SDL.h>                // For SDL core functionality (window, events).
 #include <SDL_image.h>          // For SDL_image (loading image textures).
 #include <SDL_ttf.h>            // For SDL_ttf (rendering text with fonts).
 #include <SDL_mixer.h>          // For SDL_mixer (audio playback).
 #include <vlc/vlc.h>            // For libvlc (video playback).
 #include "imgui.h"              // For ImGui (GUI framework).
 #include "imgui_impl_sdl2.h"    // For ImGui SDL2 backend (event handling).
 #include "imgui_impl_sdlrenderer2.h" // For ImGui SDL_Renderer backend (rendering).
 #include "config/config_gui.h"   // For IniEditor (configuration GUI).
 #include "config/config_loader.h" // For config loading (keybindings, settings).
 #include "render/video_player.h" // For VideoContext (video playback setup).
 #include "table/table_manager.h" // For Table, loadTableList (table management).
 #include "render/renderer.h"     // For Renderer (rendering assets).
 #include "sdl_guards.h"          // For SDL guards (resource management).
 #include "transition_manager.h"  // For TransitionManager (fade transitions).
 #include "table/asset_manager.h" // For AssetManager (loading table assets).
 #include "input/input_manager.h" // For InputManager (handling user input).
 #include "capture/screenshot_manager.h" // For ScreenshotManager (capturing screenshots).
 #include "logging.h"             // For LOG_DEBUG (debug logging).
 #include "version.h"             // For VERSION (application version).
 #include "config/config_manager.h"
 
 #define CHECK_SDL(x, msg) if (!(x)) { std::cerr << msg << ": " << SDL_GetError() << std::endl; return 1; }
 
 // Get the directory containing the executable
 std::string getExecutableDir() {
     char path[PATH_MAX];
     ssize_t count = readlink("/proc/self/exe", path, PATH_MAX);
     if (count == -1) {
         LOG_DEBUG("Warning: Couldn't determine executable path, using './'");
         return "./";
     }
     path[count] = '\0';
     std::string fullPath = std::string(path);
     size_t lastSlash = fullPath.find_last_of('/');
     if (lastSlash == std::string::npos) {
         return "./";
     }
     return fullPath.substr(0, lastSlash + 1);  // Include trailing /
 }
 
 // Check if config is valid
 bool isConfigValid() {
     // Check VPX executable
     if (VPX_EXECUTABLE_CMD.empty() || !std::filesystem::exists(VPX_EXECUTABLE_CMD)) {
         std::cerr << "Invalid VPX executable path: " << VPX_EXECUTABLE_CMD << std::endl;
         return false;
     }
 
     // Check table path for existence and .vpx files recursively
     if (VPX_TABLES_PATH.empty() || !std::filesystem::exists(VPX_TABLES_PATH)) {
         std::cerr << "Invalid table path: " << VPX_TABLES_PATH << std::endl;
         return false;
     }
     bool hasVpxFiles = false;
     for (const auto& entry : std::filesystem::recursive_directory_iterator(VPX_TABLES_PATH)) {
         if (entry.path().extension() == ".vpx") {
             hasVpxFiles = true;
             break;
         }
     }
     if (!hasVpxFiles) {
         std::cerr << "No .vpx files found in table path or subdirectories: " << VPX_TABLES_PATH << std::endl;
         return false;
     }
 
     return true;
 }
 
 // Runs initial config GUI if config is invalid, ensuring required settings are set.
 void runInitialConfig(const std::string& configPath) {
     // Initialize SDL for video and timer.
     SDLInitGuard sdlInit(SDL_INIT_VIDEO | SDL_INIT_TIMER);
     if (!sdlInit.success) {
         LOG_DEBUG("SDL_Init failed for config: " << SDL_GetError());
         exit(1);
     }
 
     // Create config window (800x500, centered).
     auto window = std::unique_ptr<SDL_Window, void(*)(SDL_Window*)>(
         SDL_CreateWindow("ASAPCabinetFE Setup",
             SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
             800, 500, SDL_WINDOW_SHOWN),
         SDL_DestroyWindow);
     if (!window) {
         LOG_DEBUG("Failed to create config window: " << SDL_GetError());
         exit(1);
     }
 
     // Create renderer for config window.
     auto renderer = std::unique_ptr<SDL_Renderer, void(*)(SDL_Renderer*)>(
         SDL_CreateRenderer(window.get(), -1, SDL_RENDERER_ACCELERATED),
         SDL_DestroyRenderer);
     if (!renderer) {
         LOG_DEBUG("Failed to create config renderer: " << SDL_GetError());
         exit(1);
     }
 
     // Initialize ImGui for config GUI.
     IMGUI_CHECKVERSION();
     ImGui::CreateContext();
     ImGui::StyleColorsDark();
     ImGui_ImplSDL2_InitForSDLRenderer(window.get(), renderer.get());
     ImGui_ImplSDLRenderer2_Init(renderer.get());
 
     // Temporarily override window dimensions for config GUI.
     int originalWidth = MAIN_WINDOW_WIDTH;
     int originalHeight = MAIN_WINDOW_HEIGHT;
     MAIN_WINDOW_WIDTH = 800;
     MAIN_WINDOW_HEIGHT = 500;
 
     // Run config editor until valid config is saved.
     bool showConfig = true;
     IniEditor configEditor(configPath, showConfig, nullptr); // Pass nullptr for ConfigManager
     while (true) {
         SDL_Event event;
         while (SDL_PollEvent(&event)) {
             ImGui_ImplSDL2_ProcessEvent(&event);
             configEditor.handleEvent(event);
             if (event.type == SDL_QUIT) {
                 std::cerr << "Config window closed without saving. Exiting..." << std::endl;
                 exit(1);
             }
         }
 
         // Render config GUI.
         ImGui_ImplSDLRenderer2_NewFrame();
         ImGui_ImplSDL2_NewFrame();
         ImGui::NewFrame();
         configEditor.drawGUI();
         ImGui::Render();
         SDL_SetRenderDrawColor(renderer.get(), 0, 0, 0, 255);
         SDL_RenderClear(renderer.get());
         ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), renderer.get());
         SDL_RenderPresent(renderer.get());
 
         // Check if config is valid after saving; loop until valid.
         if (!showConfig) {
             initialize_config(configPath);
             if (isConfigValid()) {
                 break;
             } else {
                 std::cerr << "Config still invalid. Please fix VPX.ExecutableCmd and VPX.TablesPath." << std::endl;
                 showConfig = true; // Reopen GUI if config is invalid.
             }
         }
     }
 
     // Restore original window dimensions.
     MAIN_WINDOW_WIDTH = originalWidth;
     MAIN_WINDOW_HEIGHT = originalHeight;
 
     // Cleanup ImGui.
     ImGui_ImplSDLRenderer2_Shutdown();
     ImGui_ImplSDL2_Shutdown();
     ImGui::DestroyContext();
 }
 
 // Main entry point.
 int main(int argc, char* argv[]) {
     // ----- Handle Command-Line Arguments -----
     // Check for --version flag and exit if present.
     if (argc == 2 && std::string(argv[1]) == "--version") {
         std::cout << "ASAPCabinetFE version " << PROJECT_VERSION << std::endl;
         return 0;
     }
 
     // ----- Initialize Configuration -----
     // Determine executable directory and load config.
     std::string exeDir = getExecutableDir();
     std::string configPath = exeDir + "config.ini";
     initialize_config(configPath);  // Pass full path
 
     // Check config validity and run setup if needed.
     if (!isConfigValid()) {
         std::cout << "Invalid config detected. Launching setup..." << std::endl;
         runInitialConfig(configPath);
     }
 
     // ----- Initialize SDL and Subsystems -----
     // Initialize SDL for video, timer, and audio.
     SDLInitGuard sdlInit(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_AUDIO);
     CHECK_SDL(sdlInit.success, "SDL_Init failed");
 
     // Initialize SDL_image for PNG/JPG support.
     IMGInitGuard imgInit(IMG_INIT_PNG | IMG_INIT_JPG);
     CHECK_SDL(imgInit.flags, "IMG_Init failed");
 
     // Initialize SDL_ttf for font rendering.
     TTFInitGuard ttfInit;
     CHECK_SDL(ttfInit.success, "TTF_Init failed");
 
     // Initialize SDL_mixer for audio playback.
     MixerGuard mixerGuard(44100, MIX_DEFAULT_FORMAT, 2, 2048);
     CHECK_SDL(mixerGuard.success, "Mix_OpenAudio failed");
 
     // ----- Create Windows and Renderers -----
     // Create primary window (playfield).
     auto primaryWindow = std::unique_ptr<SDL_Window, void(*)(SDL_Window*)>(
         SDL_CreateWindow("Playfield", SDL_WINDOWPOS_CENTERED_DISPLAY(MAIN_WINDOW_MONITOR), SDL_WINDOWPOS_CENTERED,
             MAIN_WINDOW_WIDTH, MAIN_WINDOW_HEIGHT, SDL_WINDOW_SHOWN | SDL_WINDOW_BORDERLESS),
         SDL_DestroyWindow);
     CHECK_SDL(primaryWindow, "Failed to create primary window");
 
     // Create primary renderer with VSync.
     auto primaryRenderer = std::unique_ptr<SDL_Renderer, void(*)(SDL_Renderer*)>(
         SDL_CreateRenderer(primaryWindow.get(), -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC),
         SDL_DestroyRenderer);
     CHECK_SDL(primaryRenderer, "Failed to create primary renderer");
 
     // Create secondary window (backglass).
     auto secondaryWindow = std::unique_ptr<SDL_Window, void(*)(SDL_Window*)>(
         SDL_CreateWindow("Backglass", SDL_WINDOWPOS_CENTERED_DISPLAY(SECOND_WINDOW_MONITOR), SDL_WINDOWPOS_CENTERED,
             SECOND_WINDOW_WIDTH, SECOND_WINDOW_HEIGHT, SDL_WINDOW_SHOWN | SDL_WINDOW_BORDERLESS),
         SDL_DestroyWindow);
     CHECK_SDL(secondaryWindow, "Failed to create secondary window");
 
     // Create secondary renderer with VSync.
     auto secondaryRenderer = std::unique_ptr<SDL_Renderer, void(*)(SDL_Renderer*)>(
         SDL_CreateRenderer(secondaryWindow.get(), -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC),
         SDL_DestroyRenderer);
     CHECK_SDL(secondaryRenderer, "Failed to create secondary renderer");
 
     // ----- Initialize ImGui -----
     // Set up ImGui for GUI rendering.
     IMGUI_CHECKVERSION();
     ImGui::CreateContext();
     ImGui::StyleColorsDark();
     ImGui_ImplSDL2_InitForSDLRenderer(primaryWindow.get(), primaryRenderer.get());
     ImGui_ImplSDLRenderer2_Init(primaryRenderer.get());
 
     // ----- Load Resources and Managers -----
     // Initialize config editor.
     bool showConfig = false;
 
     // Load font for rendering table names.
     auto font = std::unique_ptr<TTF_Font, void(*)(TTF_Font*)>(TTF_OpenFont(FONT_PATH.c_str(), FONT_SIZE), TTF_CloseFont);
     if (!font) std::cerr << "Failed to load font: " << TTF_GetError() << std::endl;
     
     // Load sound effects for table changes and launches.
     std::string tableChangeSoundPath = exeDir + TABLE_CHANGE_SOUND;
     auto tableChangeSound = std::unique_ptr<Mix_Chunk, void(*)(Mix_Chunk*)>(Mix_LoadWAV(tableChangeSoundPath.c_str()), Mix_FreeChunk);
     if (!tableChangeSound) std::cerr << "Mix_LoadWAV Error at " << tableChangeSoundPath << ": " << Mix_GetError() << std::endl;
 
     std::string tableLoadSoundPath = exeDir + TABLE_LOAD_SOUND;
     auto tableLoadSound = std::unique_ptr<Mix_Chunk, void(*)(Mix_Chunk*)>(Mix_LoadWAV(tableLoadSoundPath.c_str()), Mix_FreeChunk);
     if (!tableLoadSound) std::cerr << "Mix_LoadWAV Error at " << tableLoadSoundPath << ": " << Mix_GetError() << std::endl;
 
     // Load table list and initialize assets.
     std::vector<Table> tables = loadTableList();
     if (tables.empty()) { std::cerr << "Edit config.ini, no .vpx files found in " << VPX_TABLES_PATH << std::endl; return 1; }
 
     size_t currentIndex = 0;
     AssetManager assets(primaryRenderer.get(), secondaryRenderer.get(), font.get());
     TransitionManager transitionManager;
     InputManager inputManager;
     ScreenshotManager screenshotManager(exeDir);  // Instantiate with exeDir
     ConfigManager configManager(configPath, primaryWindow.get(), primaryRenderer.get(),
                                 secondaryWindow.get(), secondaryRenderer.get(),
                                 font, assets, currentIndex, tables);
     IniEditor configEditor(configPath, showConfig, &configManager);
     bool quit = false;
     SDL_Event event;
 
     assets.loadTableAssets(currentIndex, tables);
 
     // ----- Main Loop -----
     while (!quit) {
         // ----- Event Handling -----
         // Process SDL events (input, quit, etc.).
         while (SDL_PollEvent(&event)) {
             ImGui_ImplSDL2_ProcessEvent(&event);
             if (event.type == SDL_QUIT) quit = true;
 
             if (event.type == SDL_KEYDOWN) {
                 // Toggle config GUI visibility.
                 if (inputManager.isToggleConfig(event)) {
                     showConfig = !showConfig;
                     LOG_DEBUG("Toggled showConfig to: " << (showConfig ? 1 : 0));
                 }
 
                 // Handle events in config GUI if open.
                 if (showConfig) {
                     configEditor.handleEvent(event);
                     if (!configEditor.isCapturingKey()) {
                         continue; // Skip other input processing during key capture.
                     }
                 }
 
                 // Process input for table navigation and actions.
                 if (!showConfig) {
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
                         LOG_DEBUG("Launching: " << command);
                         int result = std::system(command.c_str());
                         if (result != 0) {
                             std::cerr << "Warning: VPX launch failed with exit code " << result << std::endl;
                         }
                     }
                     else if (inputManager.isScreenshotMode(event)) {
                         LOG_DEBUG("Entering screenshot mode for: " << tables[currentIndex].vpxFile);
                         screenshotManager.launchScreenshotMode(tables[currentIndex].vpxFile);
                     }
                     else if (inputManager.isQuit(event)) {
                         quit = true;
                         LOG_DEBUG("Quit triggered via keybind");
                     }
                 }
             }
         }
 
         // ----- ImGui Frame Setup -----
         // Prepare ImGui for rendering.
         ImGui_ImplSDLRenderer2_NewFrame();
         ImGui_ImplSDL2_NewFrame();
         ImGui::NewFrame();
 
         // ----- Update Video and Transitions -----
         // Update video textures and transition effects.
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
 
         // ----- Render Primary Window (Playfield) -----
         // Clear and render playfield content (table, wheel, name).
         SDL_SetRenderDrawColor(primaryRenderer.get(), 32, 32, 32, 255);
         SDL_RenderClear(primaryRenderer.get());
 
         SDL_Rect tableRect = {0, 0, MAIN_WINDOW_WIDTH, MAIN_WINDOW_HEIGHT};
         if (transitionManager.shouldMaskFrame()) {
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
             SDL_SetRenderDrawColor(primaryRenderer.get(), FONT_BG_COLOR.r, FONT_BG_COLOR.g, FONT_BG_COLOR.b, FONT_BG_COLOR.a);
             SDL_RenderFillRect(primaryRenderer.get(), &bgRect);
             SDL_RenderCopy(primaryRenderer.get(), assets.getTableNameTexture(), nullptr, &nameRect);
         }
 
         // Render config GUI if open.
         if (showConfig) configEditor.drawGUI();
 
         // Finalize ImGui rendering for primary window.
         ImGui::Render();
         ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), primaryRenderer.get());
         SDL_RenderPresent(primaryRenderer.get());
 
         // ----- Render Secondary Window (Backglass/DMD) -----
         // Clear and render backglass and DMD content.
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
 
         // Clean up old video players after transition.
         if (!transitionManager.isTransitionActive()) {
             assets.clearOldVideoPlayers();
         }
 
         // ----- Handle Config Changes -----
         // Reload UI if config changes are pending.
         configManager.applyConfigChanges();
     }
 
     // ----- Cleanup -----
     // Clean up video players and ImGui before exiting.
     cleanupVideoContext(assets.getTableVideoPlayer());
     cleanupVideoContext(assets.getBackglassVideoPlayer());
     cleanupVideoContext(assets.getDmdVideoPlayer());
     assets.clearOldVideoPlayers();
 
     ImGui_ImplSDLRenderer2_Shutdown();
     ImGui_ImplSDL2_Shutdown();
     ImGui::DestroyContext();
 
     return 0;
 }