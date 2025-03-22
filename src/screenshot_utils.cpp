#include "screenshot_utils.h"
#include <iostream>
#include <cstdlib>
#include <unistd.h>
#include <sys/wait.h>
#include <fstream>
#include <sstream>
#include <thread>
#include <vector>
#include "config.h"
#include <SDL2/SDL_ttf.h>

const std::string VPX_LOG_FILE = "logs/VPinballX.log";

std::string shell_escape(const std::string& str) {
    std::ostringstream escaped;
    escaped << "\"";
    for (char c : str) {
        if (c == '"' || c == '\\') escaped << "\\";
        escaped << c;
    }
    escaped << "\"";
    return escaped.str();
}

bool is_window_visible_log(const std::string& title) {
    std::ifstream log(VPX_LOG_FILE);
    if (!log.is_open()) {
        std::cerr << "Error: Could not open " << VPX_LOG_FILE << std::endl;
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

void capture_screenshot(const std::string& window_name, const std::string& output_path) {
    std::string cmd = "xdotool search --name " + shell_escape(window_name) + " | head -n 1";
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) {
        std::cerr << "Error: Failed to run xdotool search for " << window_name << "." << std::endl;
        return;
    }
    char buffer[128];
    std::string window_id;
    if (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        window_id = buffer;
        window_id.erase(window_id.find_last_not_of(" \t\r\n") + 1);
    }
    pclose(pipe);

    if (window_id.empty()) {
        std::cerr << "Warning: Window '" << window_name << "' not found." << std::endl;
        return;
    }

    cmd = "xdotool windowactivate " + window_id + " >/dev/null 2>&1";
    if (std::system(cmd.c_str()) != 0) {
        std::cerr << "Warning: Failed to activate window " << window_name << std::endl;
    }
    cmd = "xdotool windowraise " + window_id + " >/dev/null 2>&1";
    if (std::system(cmd.c_str()) != 0) {
        std::cerr << "Warning: Failed to raise window " << window_name << std::endl;
    }
    usleep(500000);

    cmd = "mkdir -p " + shell_escape(output_path.substr(0, output_path.find_last_of('/')));
    if (std::system(cmd.c_str()) != 0) {
        std::cerr << "Error: Failed to create directory for " << output_path << std::endl;
        return;
    }
    cmd = "import -window " + window_id + " " + shell_escape(output_path);
    if (std::system(cmd.c_str()) == 0) {
        std::cout << "Saved screenshot to " << output_path << std::endl;
    } else {
        std::cerr << "Error: Failed to save screenshot to " << output_path << std::endl;
    }
}

void capture_all_screenshots(const std::string& table_image, const std::string& backglass_image, 
                            const std::string& dmd_image, SDL_Window* window) {
    std::vector<std::thread> threads;

    threads.emplace_back([&]() {
        capture_screenshot("Visual Pinball Player", table_image);
    });

    if (is_window_visible_log("B2SBackglass")) {
        threads.emplace_back([&]() {
            capture_screenshot("B2SBackglass", backglass_image);
        });
    } else {
        std::cerr << "Warning: Backglass window not visible in VPX log." << std::endl;
    }

    std::string dmd_windows[] = {"FlexDMD", "PinMAME", "B2SDMD"};
    bool dmd_captured = false;
    for (const auto& dmd : dmd_windows) {
        if (is_window_visible_log(dmd)) {
            threads.emplace_back([&, dmd]() {
                capture_screenshot(dmd, dmd_image);
            });
            dmd_captured = true;
            break;
        }
    }
    if (!dmd_captured) {
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

void launch_screenshot_mode(const std::string& vpx_file) {
    std::string vpx_executable = VPX_EXECUTABLE_CMD;
    std::string table_folder = vpx_file.substr(0, vpx_file.find_last_of('/'));
    std::string table_image = table_folder + "/" + CUSTOM_TABLE_IMAGE;
    std::string backglass_image = table_folder + "/" + CUSTOM_BACKGLASS_IMAGE;
    std::string dmd_image = table_folder + "/" + CUSTOM_DMD_IMAGE;

    if (vpx_executable.empty()) {
        std::cerr << "Error: Missing VPX.ExecutableCmd in config.ini" << std::endl;
        return;
    }
    if (CUSTOM_TABLE_IMAGE.empty() || CUSTOM_BACKGLASS_IMAGE.empty() || CUSTOM_DMD_IMAGE.empty()) {
        std::cerr << "Error: Missing image paths in config.ini (TableImage, BackglassImage, DmdImage)." << std::endl;
        return;
    }

    pid_t vpx_pid = fork();
    if (vpx_pid < 0) {
        std::cerr << "Error: Fork failed." << std::endl;
        return;
    }
    if (vpx_pid == 0) {
        std::string cmd = "mkdir -p logs && " + vpx_executable + " -play " + shell_escape(vpx_file) + " > " + VPX_LOG_FILE + " 2>&1";
        std::cout << "Launching VPX screenshot mode: " << cmd << std::endl;
        execl("/bin/sh", "sh", "-c", cmd.c_str(), nullptr);
        std::cerr << "Error: execl failed." << std::endl;
        exit(1);
    }

    sleep(5);  // Wait for VPX to load

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "SDL_Init Error: " << SDL_GetError() << std::endl;
        kill(vpx_pid, SIGKILL);
        waitpid(vpx_pid, nullptr, 0);
        return;
    }

    if (TTF_Init() < 0) {
        std::cerr << "TTF_Init Error: " << TTF_GetError() << std::endl;
        SDL_Quit();
        kill(vpx_pid, SIGKILL);
        waitpid(vpx_pid, nullptr, 0);
        return;
    }

    SDL_Window* window = SDL_CreateWindow("VPX Screenshot",
                                          SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                          100, 35, SDL_WINDOW_BORDERLESS | SDL_WINDOW_ALWAYS_ON_TOP);
    if (!window) {
        std::cerr << "SDL_CreateWindow Error: " << SDL_GetError() << std::endl;
        TTF_Quit();
        SDL_Quit();
        kill(vpx_pid, SIGKILL);
        waitpid(vpx_pid, nullptr, 0);
        return;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        std::cerr << "SDL_CreateRenderer Error: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
        kill(vpx_pid, SIGKILL);
        waitpid(vpx_pid, nullptr, 0);
        return;
    }

    TTF_Font* font = TTF_OpenFont(FONT_PATH.c_str(), 14);
    if (!font) {
        std::cerr << "TTF_OpenFont Error: " << TTF_GetError() << std::endl;
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
        kill(vpx_pid, SIGKILL);
        waitpid(vpx_pid, nullptr, 0);
        return;
    }

    SDL_Color white = {255, 255, 255, 255};
    SDL_Surface* text_surface = TTF_RenderText_Solid(font, "Screenshot", white);
    if (!text_surface) {
        std::cerr << "TTF_RenderText_Solid Error: " << TTF_GetError() << std::endl;
        TTF_CloseFont(font);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
        kill(vpx_pid, SIGKILL);
        waitpid(vpx_pid, nullptr, 0);
        return;
    }
    SDL_Texture* text_texture = SDL_CreateTextureFromSurface(renderer, text_surface);
    if (!text_texture) {
        std::cerr << "SDL_CreateTextureFromSurface Error: " << SDL_GetError() << std::endl;
        SDL_FreeSurface(text_surface);
        TTF_CloseFont(font);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
        kill(vpx_pid, SIGKILL);
        waitpid(vpx_pid, nullptr, 0);
        return;
    }

    SDL_RaiseWindow(window);  // Steal focus from VPX
    std::string cmd = "xdotool search --name \"VPX Screenshot\" windowactivate >/dev/null 2>&1";
    if (std::system(cmd.c_str()) != 0) {
        std::cerr << "Warning: Failed to activate VPX Screenshot window." << std::endl;
    }

    SDL_Rect button = {0, 0, 100, 35};
    std::cout << "Screenshot mode active. Press 'S' to capture, 'Q' to quit." << std::endl;

    bool running = true;
    SDL_Event event;
    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            } else if (event.type == SDL_KEYDOWN) {
                if (event.key.keysym.sym == SDLK_s) {
                    std::cout << "Capturing screenshots..." << std::endl;
                    capture_all_screenshots(table_image, backglass_image, dmd_image, window);
                } else if (event.key.keysym.sym == SDLK_q) {
                    running = false;
                }
            } else if (event.type == SDL_MOUSEBUTTONDOWN) {
                int x = event.button.x, y = event.button.y;
                if (x >= button.x && x <= button.x + button.w && y >= button.y && y <= button.y + button.h) {
                    std::cout << "Capturing screenshots..." << std::endl;
                    capture_all_screenshots(table_image, backglass_image, dmd_image, window);
                }
            }
        }

        SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
        SDL_RenderClear(renderer);
        SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
        SDL_RenderFillRect(renderer, &button);
        SDL_Rect text_rect = {button.x + 10, button.y + 10, text_surface->w, text_surface->h};
        SDL_RenderCopy(renderer, text_texture, nullptr, &text_rect);
        SDL_RenderPresent(renderer);
        SDL_Delay(10);  // Avoid CPU hogging
    }

    std::cout << "Killing VPX processes..." << std::endl;
    std::system("pkill -9 -f VPinballX_GL >/dev/null 2>&1");
    kill(vpx_pid, SIGKILL);
    waitpid(vpx_pid, nullptr, 0);

    SDL_FreeSurface(text_surface);
    SDL_DestroyTexture(text_texture);
    TTF_CloseFont(font);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    // No TTF_Quit() or SDL_Quit()—leave SDL alive for frontend
}