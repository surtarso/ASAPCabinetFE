#include "capture/screenshot_manager.h"
#include <iostream>
#include <cstdlib>
#include <unistd.h>
#include <sys/wait.h>
#include <fstream>
#include <sstream>
#include <thread>
#include <vector>
#include "config/settings_manager.h"
#include "keybinds/keybind_manager.h" // Updated include
#include "utils/logging.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

ScreenshotManager::ScreenshotManager(const std::string& exeDir, SettingsManager* configManager, KeybindManager* keybindManager) 
    : exeDir_(exeDir), vpxLogFile(exeDir + "logs/VPinballX.log"), configManager_(configManager), keybindManager_(keybindManager) {
    LOG_DEBUG("ScreenshotManager initialized with exeDir: " << exeDir_);
    LOG_DEBUG("VPX_LOG_FILE set to: " << vpxLogFile);
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
    std::string cmd = "xdotool search --name \"" + title + "\" getwindowname >/dev/null 2>&1";
    int ret = std::system(cmd.c_str());
    bool visible = (ret == 0);
    LOG_DEBUG("X11/Wayland check for '" << title << "': " << (visible ? "visible" : "not visible"));
    return visible;
}

void ScreenshotManager::captureScreenshot(const std::string& windowName, const std::string& outputPath) {
    std::string cmd = "xdotool search --name " + shellEscape(windowName) + " | head -n 1";
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) {
        LOG_DEBUG("Error: Failed to run xdotool search for " << windowName << ".");
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
        LOG_DEBUG("Warning: Window '" << windowName << "' not found.");
        return;
    }

    cmd = "xdotool windowactivate " + windowId + " >/dev/null 2>&1";
    if (std::system(cmd.c_str()) != 0) {
        LOG_DEBUG("Warning: Failed to activate window " << windowName);
    }
    cmd = "xdotool windowraise " + windowId + " >/dev/null 2>&1";
    if (std::system(cmd.c_str()) != 0) {
        LOG_DEBUG("Warning: Failed to raise window " << windowName);
    }
    usleep(400000);

    cmd = "mkdir -p " + shellEscape(outputPath.substr(0, outputPath.find_last_of('/')));
    if (std::system(cmd.c_str()) != 0) {
        LOG_DEBUG("Error: Failed to create directory for " << outputPath);
        return;
    }
    cmd = "import -window " + windowId + " " + shellEscape(outputPath);
    if (std::system(cmd.c_str()) == 0) {
        LOG_DEBUG("Saved screenshot to " << outputPath);
    } else {
        LOG_DEBUG("Error: Failed to save screenshot to " << outputPath);
    }
}

void ScreenshotManager::captureAllScreenshots(const std::string& tableImage, const std::string& backglassImage, 
                                              const std::string& dmdImage, SDL_Window* window) {
    std::vector<std::thread> threads;

    threads.emplace_back([&]() {
        captureScreenshot("Visual Pinball Player", tableImage);
    });

    if (isWindowVisibleLog("B2SBackglass")) {
        threads.emplace_back([&]() {
            captureScreenshot("B2SBackglass", backglassImage);
        });
    } else {
        LOG_DEBUG("Warning: Backglass window not visible in VPX log.");
    }

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
        LOG_DEBUG("Warning: No visible DMD window detected.");
    }

    for (auto& thread : threads) {
        if (thread.joinable()) {
            thread.join();
        }
    }

    SDL_RaiseWindow(window);
    std::string cmd = "xdotool search --name \"VPX Screenshot\" windowactivate >/dev/null 2>&1";
    if (std::system(cmd.c_str()) != 0) {
        LOG_DEBUG("Warning: Failed to refocus VPX Screenshot window.");
    }
}

void ScreenshotManager::showHelpWindow(SDL_Window*& helpWindow, SDL_Renderer*& helpRenderer) {
    helpWindow = SDL_CreateWindow("VPX Screenshot", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 300, 100, SDL_WINDOW_SHOWN);
    if (!helpWindow) {
        LOG_DEBUG("Error: Failed to create help window: " << SDL_GetError());
        return;
    }
    helpRenderer = SDL_CreateRenderer(helpWindow, -1, SDL_RENDERER_ACCELERATED);
    if (!helpRenderer) {
        LOG_DEBUG("Error: Failed to create help renderer: " << SDL_GetError());
        SDL_DestroyWindow(helpWindow);
        helpWindow = nullptr;
        return;
    }
    SDL_SetRenderDrawColor(helpRenderer, 0, 0, 0, 255);
    SDL_RenderClear(helpRenderer);
    SDL_RenderPresent(helpRenderer);
    LOG_DEBUG("Help window created");
}

void ScreenshotManager::launchScreenshotMode(const std::string& vpxFile) {
    LOG_DEBUG("Starting launchScreenshotMode for: " << vpxFile << " with exeDir: " << exeDir_);
    const Settings& settings = configManager_->getSettings();
    std::string vpxExecutable = settings.vpxExecutableCmd;
    std::string tableFolder = vpxFile.substr(0, vpxFile.find_last_of('/'));
    std::string tableImage = tableFolder + "/" + settings.customTableImage;
    std::string backglassImage = tableFolder + "/" + settings.customBackglassImage;
    std::string dmdImage = tableFolder + "/" + settings.customDmdImage;

    // Access keybindings via KeybindManager (already updated in constructor)
    SDL_Keycode screenshotKey = configManager_->getKeybindManager().getKey("ScreenshotKey");
    SDL_Keycode screenshotQuit = configManager_->getKeybindManager().getKey("ScreenshotQuit");

    std::string logDir = exeDir_ + "logs";
    std::string mkdirCmd = "mkdir -p " + shellEscape(logDir) + " && rm -f " + vpxLogFile;
    LOG_DEBUG("Preparing logs with: " << mkdirCmd);
    if (std::system(mkdirCmd.c_str()) != 0) {
        LOG_DEBUG("Error: Failed to create logs directory or clear log: " << logDir);
        return;
    }

    pid_t vpxPid = fork();
    if (vpxPid < 0) {
        LOG_DEBUG("Error: Fork failed with errno: " << errno << " (" << strerror(errno) << ")");
        return;
    }
    if (vpxPid == 0) {
        std::string cmd = vpxExecutable + " -play " + shellEscape(vpxFile) + " > " + vpxLogFile + " 2>&1";
        LOG_DEBUG("Executing VPX command: " << cmd);
        int ret = execl("/bin/sh", "sh", "-c", cmd.c_str(), nullptr);
        LOG_DEBUG("Error: execl failed with return " << ret << ", errno: " << errno << " (" << strerror(errno) << ")");
        exit(1);
    }

    LOG_DEBUG("Waiting 4s for VPX to fully initialize");
    sleep(4); // Wait for VPX to load

    if (!isWindowVisibleLog("Visual Pinball Player")) {
        LOG_DEBUG("Aborting screenshot mode - VPX window not found after 4s");
        kill(vpxPid, SIGKILL);
        waitpid(vpxPid, nullptr, 0);
        return;
    }
    LOG_DEBUG("VPX playfield window detected after 4s.");

    // Additional delay to ensure VPX has fully settled and stolen focus
    LOG_DEBUG("Waiting an additional 1s for VPX to settle");
    sleep(1);

    int windowWidth = 215;
    int windowHeight = 35;
    SDL_Window* window = SDL_CreateWindow("VPX Screenshot",
        SDL_WINDOWPOS_CENTERED_DISPLAY(settings.mainWindowMonitor),
        SDL_WINDOWPOS_CENTERED_DISPLAY(settings.mainWindowMonitor),
        windowWidth, windowHeight, SDL_WINDOW_BORDERLESS | SDL_WINDOW_ALWAYS_ON_TOP);
    
    if (!window) {
        LOG_DEBUG("SDL_CreateWindow Error: " << SDL_GetError());
        kill(vpxPid, SIGKILL);
        waitpid(vpxPid, nullptr, 0);
        return;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        LOG_DEBUG("SDL_CreateRenderer Error: " << SDL_GetError());
        SDL_DestroyWindow(window);
        kill(vpxPid, SIGKILL);
        waitpid(vpxPid, nullptr, 0);
        return;
    }

    TTF_Font* font = TTF_OpenFont(settings.fontPath.c_str(), 14);
    if (!font) {
        LOG_DEBUG("TTF_OpenFont Error: " << TTF_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        kill(vpxPid, SIGKILL);
        waitpid(vpxPid, nullptr, 0);
        return;
    }

    SDL_Color white = {255, 255, 255, 255};
    std::string buttonText = "'" + keycodeToString(screenshotKey) + "' to Screenshot, '" +
                             keycodeToString(screenshotQuit) + "' to Quit";
    SDL_Surface* textSurface = TTF_RenderText_Solid(font, buttonText.c_str(), white);
    if (!textSurface) {
        LOG_DEBUG("TTF_RenderText_Solid Error: " << TTF_GetError());
        TTF_CloseFont(font);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        kill(vpxPid, SIGKILL);
        waitpid(vpxPid, nullptr, 0);
        return;
    }
    SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
    if (!textTexture) {
        LOG_DEBUG("SDL_CreateTextureFromSurface Error: " << SDL_GetError());
        SDL_FreeSurface(textSurface);
        TTF_CloseFont(font);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        kill(vpxPid, SIGKILL);
        waitpid(vpxPid, nullptr, 0);
        return;
    }

    SDL_RaiseWindow(window);
    SDL_SetWindowInputFocus(window); // Explicitly set input focus
    std::string cmd = "xdotool search --name \"VPX Screenshot\" windowactivate >/dev/null 2>&1";
    for (int i = 0; i < 5; i++) { // Increase to 5 attempts
        if (std::system(cmd.c_str()) == 0) {
            LOG_DEBUG("Focus stolen to VPX Screenshot window after " << i << " attempts.");
            break;
        }
        LOG_DEBUG("Focus steal attempt " << i << " failed.");
        SDL_RaiseWindow(window); // Try raising again
        SDL_SetWindowInputFocus(window); // Try setting focus again
        usleep(1000000); // Increase delay to 1 second
        if (i == 4) {
            LOG_DEBUG("Warning: Failed to steal focus to VPX Screenshot window after 5 attempts.");
        }
    }

    SDL_Rect button = {0, 0, windowWidth, windowHeight};
    LOG_DEBUG("Screenshot mode active. Press '" << keycodeToString(screenshotKey) 
              << "' to capture, '" << keycodeToString(screenshotQuit) << "' to quit.");

    bool running = true;
    SDL_Event event;
    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                LOG_DEBUG("Quit via SDL_QUIT");
                running = false;
            } else if (event.type == SDL_KEYDOWN) {
                if (!keybindManager_) {
                    LOG_DEBUG("Error: keybindManager_ is null, cannot process key events");
                    running = false;
                    break;
                }
                // Cast SDL_Event to SDL_KeyboardEvent since KeybindManager::isAction expects SDL_KeyboardEvent
                SDL_KeyboardEvent keyEvent = event.key;
                if (keybindManager_->isAction(keyEvent, "ScreenshotKey")) {
                    LOG_DEBUG("Capture key '" << keycodeToString(screenshotKey) << "' pressed");
                    captureAllScreenshots(tableImage, backglassImage, dmdImage, window);
                    // Keep screenshot mode alive
                } else if (keybindManager_->isAction(keyEvent, "ScreenshotQuit")) {
                    LOG_DEBUG("Quit key '" << keycodeToString(screenshotQuit) << "' pressed");
                    running = false; // Exit screenshot mode, which will kill VPX and return to main UI
                }
            } else if (event.type == SDL_MOUSEBUTTONDOWN) {
                int x = event.button.x, y = event.button.y;
                if (x >= button.x && x <= button.x + button.w && y >= button.y && y <= button.y + button.h) {
                    LOG_DEBUG("Capturing screenshots via mouse click...");
                    captureAllScreenshots(tableImage, backglassImage, dmdImage, window);
                    // Keep screenshot mode alive
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
        SDL_Delay(10);
    }

    LOG_DEBUG("Killing VPX processes...");
    int pkillResult = std::system("pkill -9 -f VPinballX_GL >/dev/null 2>&1");
    if (pkillResult != 0 && pkillResult != 256) {
        LOG_DEBUG("Warning: pkill failed with code " << pkillResult);
    }
    kill(vpxPid, SIGKILL);
    waitpid(vpxPid, nullptr, 0);

    SDL_FreeSurface(textSurface);
    SDL_DestroyTexture(textTexture);
    TTF_CloseFont(font);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);

    while (SDL_PollEvent(&event)) {
        LOG_DEBUG("Draining leftover event: type " << event.type);
    }

    LOG_DEBUG("Screenshot mode exited");
}

std::string ScreenshotManager::keycodeToString(SDL_Keycode key) {
    const char* keyName = SDL_GetKeyName(key);
    return std::string(keyName);
}