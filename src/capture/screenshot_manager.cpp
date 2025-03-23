#include "capture/screenshot_manager.h"
#include <iostream>
#include <cstdlib>
#include <unistd.h>
#include <sys/wait.h>
#include <fstream>
#include <sstream>
#include <thread>
#include <vector>
#include "config/config_loader.h"
#include <SDL2/SDL_ttf.h>

ScreenshotManager::ScreenshotManager(const std::string& exeDir) 
    : vpxLogFile(exeDir + "logs/VPinballX.log") {  // Set log path with exeDir
    // std::cout << "ScreenshotManager initialized with VPX_LOG_FILE: " << vpxLogFile << std::endl;
}

std::string ScreenshotManager::shellEscape(const std::string& str) {
    std::ostringstream escaped;
    escaped << "\"";
    for (char c : str) {
        if (c == '"' || c == '\\') escaped << "\\";
        escaped << c;
    }
    escaped << "\"";
    return escaped.str();
}

bool ScreenshotManager::isWindowVisibleLog(const std::string& title) {
    std::ifstream log(vpxLogFile);
    if (!log.is_open()) {
        std::cerr << "Error: Could not open " << vpxLogFile << std::endl;
        return false;
    }
    std::string line;
    while (std::getline(log, line)) {
        if (line.find("Window initialized:") != std::string::npos &&
            line.find("title=" + title) != std::string::npos &&
            line.find("visible=1") != std::string::npos) {
            return true;
        }
    }
    return false;
}

void ScreenshotManager::captureScreenshot(const std::string& windowName, const std::string& outputPath) {
    std::string cmd = "xdotool search --name " + shellEscape(windowName) + " | head -n 1";
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) {
        std::cerr << "Error: Failed to run xdotool search for " << windowName << "." << std::endl;
        return;
    }
    char buffer[128];
    std::string windowId;
    if (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        windowId = buffer;
        windowId.erase(windowId.find_last_not_of(" \t\r\n") + 1);
    }
    pclose(pipe);

    if (windowId.empty()) {
        std::cerr << "Warning: Window '" << windowName << "' not found." << std::endl;
        return;
    }

    cmd = "xdotool windowactivate " + windowId + " >/dev/null 2>&1";
    if (std::system(cmd.c_str()) != 0) {
        std::cerr << "Warning: Failed to activate window " << windowName << std::endl;
    }
    cmd = "xdotool windowraise " + windowId + " >/dev/null 2>&1";
    if (std::system(cmd.c_str()) != 0) {
        std::cerr << "Warning: Failed to raise window " << windowName << std::endl;
    }
    usleep(400000);

    cmd = "mkdir -p " + shellEscape(outputPath.substr(0, outputPath.find_last_of('/')));
    if (std::system(cmd.c_str()) != 0) {
        std::cerr << "Error: Failed to create directory for " << outputPath << std::endl;
        return;
    }
    cmd = "import -window " + windowId + " " + shellEscape(outputPath);
    if (std::system(cmd.c_str()) == 0) {
        std::cout << "Saved screenshot to " << outputPath << std::endl;
    } else {
        std::cerr << "Error: Failed to save screenshot to " << outputPath << std::endl;
    }
}

void ScreenshotManager::captureAllScreenshots(const std::string& tableImage, const std::string& backglassImage, 
                                              const std::string& dmdImage, SDL_Window* window) {
    std::vector<std::thread> threads;

    threads.emplace_back([&]() {
        captureScreenshot("Visual Pinball Player", tableImage); // add PUPPlayfield
    });

    if (isWindowVisibleLog("B2SBackglass")) { // add PUPBackglass
        threads.emplace_back([&]() {
            captureScreenshot("B2SBackglass", backglassImage);
        });
    } else {
        std::cerr << "Warning: Backglass window not visible in VPX log." << std::endl;
    }

    // PUPTopper too

    std::string dmdWindows[] = {"FlexDMD", "PinMAME", "B2SDMD", "PUPDMD", "PUPFullDMD"};
    bool dmdCaptured = false;
    for (const auto& dmd : dmdWindows) {
        if (isWindowVisibleLog(dmd)) {
            threads.emplace_back([&, dmd]() {
                captureScreenshot(dmd, dmdImage);
            });
            dmdCaptured = true;
            break;
        }
    }
    if (!dmdCaptured) {
        std::cerr << "Warning: No visible DMD window detected." << std::endl;
    }

    for (auto& thread : threads) {
        if (thread.joinable()) {
            thread.join();
        }
    }

    SDL_RaiseWindow(window);  // Keep focus on screenshot window
    std::string cmd = "xdotool search --name \"VPX Screenshot\" windowactivate >/dev/null 2>&1";
    if (std::system(cmd.c_str()) != 0) {
        std::cerr << "Warning: Failed to refocus VPX Screenshot window." << std::endl;
    }
}

void ScreenshotManager::launchScreenshotMode(const std::string& vpxFile) {
    std::string vpxExecutable = VPX_EXECUTABLE_CMD;
    std::string tableFolder = vpxFile.substr(0, vpxFile.find_last_of('/'));
    std::string tableImage = tableFolder + "/" + CUSTOM_TABLE_IMAGE;
    std::string backglassImage = tableFolder + "/" + CUSTOM_BACKGLASS_IMAGE;
    std::string dmdImage = tableFolder + "/" + CUSTOM_DMD_IMAGE;

    if (vpxExecutable.empty()) {
        std::cerr << "Error: Missing VPX.ExecutableCmd in config.ini" << std::endl;
        return;
    }
    if (CUSTOM_TABLE_IMAGE.empty() || CUSTOM_BACKGLASS_IMAGE.empty() || CUSTOM_DMD_IMAGE.empty()) {
        std::cerr << "Error: Missing image paths in config.ini (TableImage, BackglassImage, DmdImage)." << std::endl;
        return;
    }

    pid_t vpxPid = fork();
    if (vpxPid < 0) {
        std::cerr << "Error: Fork failed." << std::endl;
        return;
    }
    if (vpxPid == 0) {
        std::string cmd = "mkdir -p logs && " + vpxExecutable + " -play " + shellEscape(vpxFile) + " > " + vpxLogFile + " 2>&1";
        std::cout << "Launching VPX screenshot mode: " << cmd << std::endl;
        execl("/bin/sh", "sh", "-c", cmd.c_str(), nullptr);
        std::cerr << "Error: execl failed." << std::endl;
        exit(1);
    }

    sleep(4);  // Wait for VPX to load

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "SDL_Init Error: " << SDL_GetError() << std::endl;
        kill(vpxPid, SIGKILL);
        waitpid(vpxPid, nullptr, 0);
        return;
    }

    if (TTF_Init() < 0) {
        std::cerr << "TTF_Init Error: " << TTF_GetError() << std::endl;
        SDL_Quit();
        kill(vpxPid, SIGKILL);
        waitpid(vpxPid, nullptr, 0);
        return;
    }

    int windowWidth = 215;
    int windowHeight = 35;
    SDL_Window* window = SDL_CreateWindow("VPX Screenshot",
        SDL_WINDOWPOS_CENTERED_DISPLAY(MAIN_WINDOW_MONITOR),
        SDL_WINDOWPOS_CENTERED_DISPLAY(MAIN_WINDOW_MONITOR),
        windowWidth, windowHeight, SDL_WINDOW_BORDERLESS | SDL_WINDOW_ALWAYS_ON_TOP);
    
        if (!window) {
            std::cerr << "SDL_CreateWindow Error: " << SDL_GetError() << std::endl;
            TTF_Quit();
            SDL_Quit();
            kill(vpxPid, SIGKILL);
            waitpid(vpxPid, nullptr, 0);
        return;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        std::cerr << "SDL_CreateRenderer Error: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
        kill(vpxPid, SIGKILL);
        waitpid(vpxPid, nullptr, 0);
        return;
    }

    TTF_Font* font = TTF_OpenFont(FONT_PATH.c_str(), 14);
    if (!font) {
        std::cerr << "TTF_OpenFont Error: " << TTF_GetError() << std::endl;
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
        kill(vpxPid, SIGKILL);
        waitpid(vpxPid, nullptr, 0);
        return;
    }

    SDL_Color white = {255, 255, 255, 255};
    // SDL_Surface* textSurface = TTF_RenderText_Solid(font, "Screenshot", white);
    std::string buttonText = "'" + keycodeToString(KEY_SCREENSHOT_KEY) + "' to Screenshot, '" +
                         keycodeToString(KEY_SCREENSHOT_QUIT) + "' to Quit";
    SDL_Surface* textSurface = TTF_RenderText_Solid(font, buttonText.c_str(), white);
    if (!textSurface) {
        std::cerr << "TTF_RenderText_Solid Error: " << TTF_GetError() << std::endl;
        TTF_CloseFont(font);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
        kill(vpxPid, SIGKILL);
        waitpid(vpxPid, nullptr, 0);
        return;
    }
    SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
    if (!textTexture) {
        std::cerr << "SDL_CreateTextureFromSurface Error: " << SDL_GetError() << std::endl;
        SDL_FreeSurface(textSurface);
        TTF_CloseFont(font);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
        kill(vpxPid, SIGKILL);
        waitpid(vpxPid, nullptr, 0);
        return;
    }

    SDL_RaiseWindow(window);  // Steal focus from VPX
    std::string cmd = "xdotool search --name \"VPX Screenshot\" windowactivate >/dev/null 2>&1";
    if (std::system(cmd.c_str()) != 0) {
        std::cerr << "Warning: Failed to activate VPX Screenshot window." << std::endl;
    }

    SDL_Rect button = {0, 0, windowWidth, windowHeight};
    std::cout << "Screenshot mode active. Press '" << keycodeToString(KEY_SCREENSHOT_KEY) 
              << "' to capture, '" << keycodeToString(KEY_SCREENSHOT_QUIT) << "' to quit." << std::endl;

    bool running = true;
    SDL_Event event;
    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            } else if (event.type == SDL_KEYDOWN) {
                if (event.key.keysym.sym == KEY_SCREENSHOT_KEY) {
                    std::cout << "Capturing screenshots..." << std::endl;
                    captureAllScreenshots(tableImage, backglassImage, dmdImage, window);
                } else if (event.key.keysym.sym == KEY_SCREENSHOT_QUIT) {
                    running = false;
                }
            } else if (event.type == SDL_MOUSEBUTTONDOWN) {
                int x = event.button.x, y = event.button.y;
                if (x >= button.x && x <= button.x + button.w && y >= button.y && y <= button.y + button.h) {
                    std::cout << "Capturing screenshots..." << std::endl;
                    captureAllScreenshots(tableImage, backglassImage, dmdImage, window);
                }
            }
        }

        SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
        SDL_RenderClear(renderer);
        SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
        SDL_RenderFillRect(renderer, &button);
        SDL_Rect textRect = {button.x + 10, button.y + 10, textSurface->w, textSurface->h};
        SDL_RenderCopy(renderer, textTexture, nullptr, &textRect);
        SDL_RenderPresent(renderer);
        SDL_Delay(10);  // Avoid CPU hogging
    }

    std::cout << "Killing VPX processes..." << std::endl;
    int pkillResult = std::system("pkill -9 -f VPinballX_GL >/dev/null 2>&1");
    if (pkillResult != 0 && pkillResult != 256) {  // 256 means "no processes matched"
        std::cerr << "Warning: pkill failed with code " << pkillResult << std::endl;
    }
    
    kill(vpxPid, SIGKILL);
    waitpid(vpxPid, nullptr, 0);

    SDL_FreeSurface(textSurface);
    SDL_DestroyTexture(textTexture);
    TTF_CloseFont(font);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    // No TTF_Quit() or SDL_Quit()—leave SDL alive for frontend
}

std::string ScreenshotManager::keycodeToString(SDL_Keycode key) {
    const char* keyName = SDL_GetKeyName(key);
    return std::string(keyName);
}