#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <iostream>
#include <string>
#include <cstdlib>
#include <unistd.h>
#include <sys/wait.h>
#include <fstream>
#include <sstream>
#include <thread>  // for parallel execution
#include <vector>  // for thread management

const std::string CONFIG_FILE = "config.ini";
const std::string VPX_LOG_FILE = "logs/VPinballX.log";

// Function to escape a string for shell use
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

// Function to read config.ini with improved parsing
std::string get_ini_value(const std::string& section, const std::string& key) {
    std::ifstream file(CONFIG_FILE);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open " << CONFIG_FILE << " in current directory: " << getcwd(nullptr, 0) << std::endl;
        return "";
    }
    std::string line, result;
    bool in_section = false;

    while (std::getline(file, line)) {
        line.erase(0, line.find_first_not_of(" \t"));
        line.erase(line.find_last_not_of(" \t\r\n") + 1);

        if (line.empty() || line[0] == ';') continue;

        if (line.find("[" + section + "]") == 0) {
            in_section = true;
            continue;
        }
        if (in_section && line.find("[") == 0) break;

        if (in_section) {
            size_t eq_pos = line.find("=");
            if (eq_pos != std::string::npos) {
                std::string parsed_key = line.substr(0, eq_pos);
                parsed_key.erase(parsed_key.find_last_not_of(" \t") + 1);
                if (parsed_key == key) {
                    result = line.substr(eq_pos + 1);
                    result.erase(0, result.find_first_not_of(" \t"));
                    result.erase(result.find_last_not_of(" \t\r\n") + 1);
                    break;
                }
            }
        }
    }
    std::cout << "Read " << section << "." << key << " = '" << result << "'" << std::endl;
    return result;
}

// Function to check if a window is visible in the VPX log
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

// Function to capture a screenshot using xdotool and import
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

// Function to capture all three screenshots in parallel and refocus the SDL window
void capture_all_screenshots(const std::string& table_image, const std::string& backglass_image, const std::string& dmd_image, SDL_Window* window) {
    std::vector<std::thread> threads;

    // Capture table screenshot
    threads.emplace_back([&]() {
        capture_screenshot("Visual Pinball Player", table_image);
    });

    // Capture backglass screenshot if visible
    if (is_window_visible_log("B2SBackglass")) {
        threads.emplace_back([&]() {
            capture_screenshot("B2SBackglass", backglass_image);
        });
    } else {
        std::cerr << "Warning: Backglass window not visible in VPX log." << std::endl;
    }

    // Capture DMD screenshot (first visible window)
    std::string dmd_windows[] = {"FlexDMD", "PinMAME", "B2SDMD"};
    bool dmd_captured = false;
    for (const auto& dmd : dmd_windows) {
        if (is_window_visible_log(dmd)) {
            threads.emplace_back([&, dmd]() { // Capture dmd in lambda to avoid lifetime issues
                capture_screenshot(dmd, dmd_image);
            });
            dmd_captured = true;
            break;
        }
    }
    if (!dmd_captured) {
        std::cerr << "Warning: No visible DMD window detected." << std::endl;
    }

    // Wait for all threads to complete
    for (auto& thread : threads) {
        if (thread.joinable()) {
            thread.join();
        }
    }

    // Refocus the SDL window after all screenshots are done
    SDL_RaiseWindow(window);
    std::string cmd = "xdotool search --name \"VPX Screenshot\" windowactivate >/dev/null 2>&1";
    if (std::system(cmd.c_str()) != 0) {
        std::cerr << "Warning: Failed to refocus VPX Screenshot window." << std::endl;
    }
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " /path/to/table.vpx" << std::endl;
        return 1;
    }

    std::string table_path = argv[1];
    std::string table_folder = table_path.substr(0, table_path.find_last_of('/'));

    // Load config
    std::string vpx_executable = get_ini_value("VPX", "ExecutableCmd");
    std::string table_image = table_folder + "/" + get_ini_value("CustomMedia", "TableImage");
    std::string backglass_image = table_folder + "/" + get_ini_value("CustomMedia", "BackglassImage");
    std::string dmd_image = table_folder + "/" + get_ini_value("CustomMedia", "DmdImage");

    if (vpx_executable.empty()) {
        std::cerr << "Error: config.ini not found or missing ExecutableCmd." << std::endl;
        return 1;
    }
    if (table_image.empty() || backglass_image.empty() || dmd_image.empty()) {
        std::cerr << "Error: Missing image paths in config.ini (TableImage, BackglassImage, DmdImage)." << std::endl;
        return 1;
    }

    // Launch VPX as a subprocess
    pid_t vpx_pid = fork();
    if (vpx_pid < 0) {
        std::cerr << "Error: Fork failed." << std::endl;
        return 1;
    }
    if (vpx_pid == 0) {
        std::string cmd = "mkdir -p logs && " + vpx_executable + " -play " + shell_escape(table_path) + " > " + VPX_LOG_FILE + " 2>&1";
        std::cout << "Launching VPX with command: " << cmd << std::endl;
        execl("/bin/sh", "sh", "-c", cmd.c_str(), nullptr);
        std::cerr << "Error: execl failed." << std::endl;
        exit(1);
    }
    sleep(5);

    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "SDL_Init Error: " << SDL_GetError() << std::endl;
        return 1;
    }

    if (TTF_Init() < 0) {
        std::cerr << "TTF_Init Error: " << TTF_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow("VPX Screenshot",
                                          SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                          100, 35, SDL_WINDOW_BORDERLESS | SDL_WINDOW_ALWAYS_ON_TOP);
    if (!window) {
        std::cerr << "SDL_CreateWindow Error: " << SDL_GetError() << std::endl;
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        std::cerr << "SDL_CreateRenderer Error: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    TTF_Font* font = TTF_OpenFont("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 14);
    if (!font) {
        std::cerr << "TTF_OpenFont Error: " << TTF_GetError() << " (Check font path)" << std::endl;
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    SDL_Rect button = {0, 0, 100, 35};
    SDL_Color white = {255, 255, 255, 255};
    SDL_Surface* text_surface = TTF_RenderText_Solid(font, "Screenshot", white);
    if (!text_surface) {
        std::cerr << "TTF_RenderText_Solid Error: " << TTF_GetError() << std::endl;
        TTF_CloseFont(font);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
        return 1;
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
        return 1;
    }

    std::cout << "Window created. Press 's' to take screenshots or 'q' to quit." << std::endl;

    bool running = true;
    SDL_Event event;
    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                std::cout << "Window closed. Exiting..." << std::endl;
                running = false;
            } else if (event.type == SDL_MOUSEBUTTONDOWN) {
                int x = event.button.x;
                int y = event.button.y;
                if (x >= button.x && x <= button.x + button.w &&
                    y >= button.y && y <= button.y + button.h) {
                    std::cout << "Capturing all screenshots..." << std::endl;
                    capture_all_screenshots(table_image, backglass_image, dmd_image, window);
                }
            } else if (event.type == SDL_KEYDOWN) {
                if (event.key.keysym.sym == SDLK_s) {
                    std::cout << "Capturing all screenshots..." << std::endl;
                    capture_all_screenshots(table_image, backglass_image, dmd_image, window);
                } else if (event.key.keysym.sym == SDLK_q) {
                    std::cout << "Quit key pressed. Exiting..." << std::endl;
                    running = false;
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
    }

    // Kill VPX and related processes with fire
    std::cout << "Killing VPX processes..." << std::endl;
    std::system("pkill -9 -f VPinballX_GL"); // Kill all processes matching VPinballX_GL
    kill(vpx_pid, SIGKILL); // Kill the forked PID just in case
    waitpid(vpx_pid, nullptr, 0);

    SDL_FreeSurface(text_surface);
    SDL_DestroyTexture(text_texture);
    TTF_CloseFont(font);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();
    return 0;
}