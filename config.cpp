#include <SDL.h>
#include <SDL_ttf.h>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <iostream>

struct ConfigSection {
    std::map<std::string, std::string> keyValues; // Key-value pairs
};

/**
 * @class IniEditor
 * @brief A class to edit INI configuration files with a graphical user interface using SDL2.
 *
 * The IniEditor class provides a graphical interface for editing INI configuration files.
 * It uses SDL2 for rendering the UI and handling events, and SDL_ttf for text rendering.
 *
 * @details
 * The class supports loading and saving INI files, displaying sections and key-value pairs,
 * and providing tooltips for explanations of each configuration key. The user can interact
 * with the UI to select sections, edit values, and save changes.
 *
 * @note
 * - The class requires SDL2 and SDL_ttf libraries.
 * - The font file path is hardcoded to "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf".
 * - The class assumes a specific window size and layout for the UI elements.
 *
 * @example
 * IniEditor editor("config.ini");
 * editor.run();
 */
class IniEditor {
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    TTF_Font* font = nullptr;
    std::map<std::string, ConfigSection> iniData; // Section -> key-value map
    std::string currentSection;
    std::vector<std::string> sections; // For dropdown
    std::map<std::string, std::string> explanations; // Tooltips
    int scrollOffset = 0; // For scrolling
    bool running = true;
    std::string activeField; // Key of the currently selected text field (empty if none)
    bool dropdownOpen = false; // Whether the section dropdown is expanded
    int dropdownHoverIndex = -1; // Hovered section in dropdown (-1 if none)
    std::string typedInput; // Buffer for keyboard input
    std::string tooltipKey; // Key of the field whose tooltip is active (empty if none)

public:
    IniEditor(const std::string& iniFile) {
        // Initialize SDL2
        if (SDL_Init(SDL_INIT_VIDEO) < 0) {
            std::cerr << "SDL_Init failed: " << SDL_GetError() << std::endl;
            running = false;
            return;
        }

        if (TTF_Init() < 0) {
            std::cerr << "TTF_Init failed: " << TTF_GetError() << std::endl;
            SDL_Quit();
            running = false;
            return;
        }

        window = SDL_CreateWindow("ASAPCabinetFE Config", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 600, 400, 0);
        if (!window) {
            std::cerr << "SDL_CreateWindow failed: " << SDL_GetError() << std::endl;
            TTF_Quit();
            SDL_Quit();
            running = false;
            return;
        }

        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
        if (!renderer) {
            std::cerr << "SDL_CreateRenderer failed: " << SDL_GetError() << std::endl;
            SDL_DestroyWindow(window);
            TTF_Quit();
            SDL_Quit();
            running = false;
            return;
        }

        font = TTF_OpenFont("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 12);
        if (!font) {
            std::cerr << "TTF_OpenFont failed: " << TTF_GetError() << std::endl;
            SDL_DestroyRenderer(renderer);
            SDL_DestroyWindow(window);
            TTF_Quit();
            SDL_Quit();
            running = false;
            return;
        }

        // Load INI file
        loadIniFile(iniFile);
        if (sections.empty()) {
            std::cerr << "No sections found in " << iniFile << std::endl;
            currentSection = "";
        } else {
            currentSection = sections[0];
        }
        initExplanations();
    }

    ~IniEditor() {
        if (font) TTF_CloseFont(font);
        if (renderer) SDL_DestroyRenderer(renderer);
        if (window) SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
    }

    void run() {
        if (!running) {
            std::cerr << "Cannot run: Initialization failed" << std::endl;
            return;
        }
        while (running) {
            handleEvents();
            render();
            SDL_Delay(16); // ~60 FPS
        }
    }

private:
    void loadIniFile(const std::string& filename) {
        std::ifstream file(filename); // Open the INI file
        if (!file.is_open()) {
            std::cerr << "Failed to open " << filename << std::endl; // Error if file cannot be opened
            return;
        }
        std::string line;
        while (std::getline(file, line)) {
            line.erase(0, line.find_first_not_of(" \t")); // Trim leading whitespace
            if (line.empty() || line[0] == ';') continue; // Skip comments/blank lines
            if (line[0] == '[' && line.back() == ']') { // Section header
                currentSection = line.substr(1, line.size() - 2); // Extract section name
                sections.push_back(currentSection); // Add section to list
                iniData[currentSection] = ConfigSection(); // Initialize section in map
            } else if (!currentSection.empty() && line.find('=') != std::string::npos) { // Key-value pair
                auto pos = line.find('='); // Find '=' character
                std::string key = line.substr(0, pos); // Extract key
                std::string value = line.substr(pos + 1); // Extract value
                key.erase(key.find_last_not_of(" \t") + 1); // Trim trailing whitespace from key
                value.erase(0, value.find_first_not_of(" \t")); // Trim leading whitespace from value
                iniData[currentSection].keyValues[key] = value; // Store key-value pair in current section
            }
        }
        file.close(); // Close the file
    }

    void saveIniFile(const std::string& filename) {
        std::vector<std::string> lines; // Vector to store lines of the file
        std::ifstream inFile(filename); // Open the file for reading
        if (!inFile.is_open()) { // Check if the file is opened successfully
            std::cerr << "Failed to read " << filename << " for saving" << std::endl;
            return;
        }
        std::string line, currentSection; // Variables to store current line and section
        while (std::getline(inFile, line)) { // Read the file line by line
            std::string trimmed = line; // Copy the line to a new string
            trimmed.erase(0, trimmed.find_first_not_of(" \t")); // Trim leading whitespace
            if (trimmed.empty() || trimmed[0] == ';') { // Check if the line is empty or a comment
                lines.push_back(line); // Add the line to the vector
            } else if (trimmed[0] == '[' && trimmed.back() == ']') { // Check if the line is a section header
                currentSection = trimmed.substr(1, trimmed.size() - 2); // Extract the section name
                lines.push_back(line); // Add the section header to the vector
            } else if (!currentSection.empty() && line.find('=') != std::string::npos) { // Check if the line is a key-value pair
                auto pos = line.find('='); // Find the position of '='
                std::string key = line.substr(0, pos); // Extract the key
                key.erase(key.find_last_not_of(" \t") + 1); // Trim trailing whitespace from the key
                if (iniData[currentSection].keyValues.count(key)) { // Check if the key exists in the current section
                    lines.push_back(key + " = " + iniData[currentSection].keyValues[key]); // Add the updated key-value pair to the vector
                } else {
                    lines.push_back(line); // Add the original line to the vector
                }
            } else {
                lines.push_back(line); // Add the line to the vector
            }
        }
        inFile.close(); // Close the input file

        std::ofstream outFile(filename); // Open the file for writing
        if (!outFile.is_open()) { // Check if the file is opened successfully
            std::cerr << "Failed to write " << filename << std::endl;
            return;
        }
        for (const auto& l : lines) outFile << l << "\n"; // Write each line to the output file
        outFile.close(); // Close the output file
    }

    void initExplanations() {
        explanations["TablesPath"] = "Specifies the absolute path to the folder containing VPX table files.\n1 - Must be a full path (e.g., /home/user/tables/).\n2 - Ensure the folder contains your table folders with .vpx table files.";
        explanations["ExecutableCmd"] = "Defines the absolute path to the VPinballX executable.\n1 - Should point to the VPinballX_GL binary or equivalent.\n2 - Verify the file is executable on your system.";
        explanations["StartArgs"] = "Optional command-line arguments to prepend to the executable.\n1 - Useful for settings like DRI_PRIME=1 or gamemoderun.\n2 - Leave blank if no extra args are needed.";
        explanations["EndArgs"] = "Optional arguments to append after the table file in the command.\n1 - Typically empty unless specific VPX options are required.\n2 - Syntax follows command-line conventions.";
        explanations["TableImage"] = "Relative path to the table's preview image.\n1 - Stored under the table folder (e.g., images/table.png).\n2 - Supports PNG; overridden by TableVideo if present.";
        explanations["BackglassImage"] = "Relative path to the backglass image.\n1 - Located in the table folder (e.g., images/backglass.png).\n2 - PNG; takes lower priority than BackglassVideo.";
        explanations["WheelImage"] = "Relative path to the wheel image for the table.\n1 - Example: images/wheel.png.\n2 - Used in UI selection menus; PNG supported.";
        explanations["DmdImage"] = "Relative path to the DMD or marquee image.\n1 - E.g., images/marquee.png; used if no DmdVideo is set.\n2 - Acts as a fallback or marquee overlay.";
        explanations["TableVideo"] = "Relative path to the table preview video.\n1 - E.g., video/table.mp4; overrides TableImage if present.\n2 - Must be MP4 format.";
        explanations["BackglassVideo"] = "Relative path to the backglass video.\n1 - E.g., video/backglass.mp4; takes priority over BackglassImage.\n2 - MP4 only.";
        explanations["DmdVideo"] = "Relative path to the DMD video.\n1 - E.g., video/dmd.mp4; overrides DmdImage if both exist.\n2 - Defaults to video/dmd.mp4 if field is empty.";
        explanations["MainMonitor"] = "Index of the monitor for the table playfield window.\n1 - Starts at 0 or 1 depending on system (check your setup).\n2 - Match this with your VPX display settings.";
        explanations["MainWidth"] = "Width of the main window in pixels.\n1 - Typically matches monitor resolution (e.g., 1080).\n2 - Adjust for custom window sizing.";
        explanations["MainHeight"] = "Height of the main window in pixels.\n1 - E.g., 1920 for vertical playfield.\n2 - Should align with VPX table dimensions.";
        explanations["SecondMonitor"] = "Index of the monitor for the backglass/DMD window.\n1 - Set to 0 or 1 based on your multi-monitor setup.\n2 - Can share a monitor with MainMonitor if needed.";
        explanations["SecondWidth"] = "Width of the secondary window in pixels.\n1 - E.g., 1024; fit it to your backglass+DMD layout.\n2 - Keep within monitor bounds.";
        explanations["SecondHeight"] = "Height of the secondary window in pixels.\n1 - E.g., 1024; accommodates backglass and DMD.\n2 - Adjust based on MediaDimensions.";
        explanations["Path"] = "Absolute path to the font file used in the UI.\n1 - E.g., /usr/share/fonts/truetype/dejavu/DejaVuSans.ttf.\n2 - Must be a valid TTF file accessible by the system.";
        explanations["Size"] = "Font size in points for text rendering.\n1 - E.g., 28; adjust for readability.\n2 - Larger values increase text size.";
        explanations["WheelImageSize"] = "Size of the wheel image in pixels.\n1 - E.g., 350; scales the image to this square size.\n2 - Keep art resolution close to this value.";
        explanations["WheelImageMargin"] = "Margin around the wheel image in pixels.\n1 - E.g., 24; adds spacing in the UI.\n2 - Increase for more padding.";
        explanations["BackglassWidth"] = "Width of the backglass media in pixels.\n1 - E.g., 1024; fits within SecondWidth.\n2 - Match your backglass art resolution.";
        explanations["BackglassHeight"] = "Height of the backglass media in pixels.\n1 - E.g., 768; leaves room for DMD below.\n2 - Adjust to your design.";
        explanations["DmdWidth"] = "Width of the DMD media in pixels.\n1 - E.g., 1024; aligns with backglass width.\n2 - Keep consistent with SecondWidth.";
        explanations["DmdHeight"] = "Height of the DMD media in pixels.\n1 - E.g., 256; fits under backglass in window.\n2 - Scale DMD art accordingly.";
    }

    void handleEvents() {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_QUIT:
                    running = false;
                    break;

                case SDL_MOUSEBUTTONDOWN:
                    if (event.button.button == SDL_BUTTON_LEFT) {
                        int mouseX = event.button.x;
                        int mouseY = event.button.y;

                        // Section dropdown
                        if (mouseX >= 10 && mouseX <= 200 && mouseY >= 10 && mouseY <= 30) {
                            dropdownOpen = !dropdownOpen;
                            break;
                        }

                        if (dropdownOpen) {
                            int y = 40;
                            for (size_t i = 0; i < sections.size(); ++i) {
                                if (mouseX >= 10 && mouseX <= 200 && mouseY >= y && mouseY <= y + 20) {
                                    currentSection = sections[i];
                                    dropdownOpen = false;
                                    activeField.clear();
                                    scrollOffset = 0;
                                    break;
                                }
                                y += 20;
                            }
                            if (!dropdownOpen) break;
                        }

                        // Text field clicks
                        int y = 50;
                        if (iniData.count(currentSection)) { // Check if section exists
                            for (const auto& [key, value] : iniData[currentSection].keyValues) {
                                if (y + 20 - scrollOffset >= 40 && y - scrollOffset <= 400) {
                                    if (mouseX >= 150 && mouseX <= 450 && mouseY >= y - scrollOffset && mouseY <= y + 20 - scrollOffset) {
                                        activeField = key;
                                        typedInput = value;
                                        SDL_StartTextInput();
                                        break;
                                    }
                                }
                                y += 30;
                            }
                        }

                        // Tooltip clicks
                        y = 50;
                        if (iniData.count(currentSection)) {
                            for (const auto& [key, value] : iniData[currentSection].keyValues) {
                                if (y + 20 - scrollOffset >= 40 && y - scrollOffset <= 400) {
                                    if (mouseX >= 120 && mouseX <= 130 && mouseY >= y - scrollOffset && mouseY <= y + 20 - scrollOffset) {
                                        if (explanations.count(key)) {
                                            tooltipKey = key;
                                        }
                                    }
                                }
                                y += 30;
                            }
                        }

                        // Close tooltip
                        if (!tooltipKey.empty() && (mouseX < 150 || mouseX > 450 || mouseY < 50 || mouseY > 350)) {
                            tooltipKey.clear();
                        }

                        // Save button
                        SDL_Rect saveBtn = {10, 360, 55, 25};
                        if (mouseX >= saveBtn.x && mouseX <= saveBtn.x + saveBtn.w &&
                            mouseY >= saveBtn.y && mouseY <= saveBtn.y + saveBtn.h) {
                            saveIniFile("config.ini");
                        }

                        // Exit button
                        SDL_Rect exitBtn = {75, 360, 55, 25};
                        if (mouseX >= exitBtn.x && mouseX <= exitBtn.x + exitBtn.w &&
                            mouseY >= exitBtn.y && mouseY <= exitBtn.y + exitBtn.h) {
                            running = false;
                        }
                    }
                    break;

                case SDL_MOUSEMOTION:
                    if (dropdownOpen) {
                        int mouseX = event.motion.x;
                        int mouseY = event.motion.y;
                        dropdownHoverIndex = -1;
                        int y = 40;
                        for (size_t i = 0; i < sections.size(); ++i) {
                            if (mouseX >= 10 && mouseX <= 200 && mouseY >= y && mouseY <= y + 20) {
                                dropdownHoverIndex = i;
                                break;
                            }
                            y += 20;
                        }
                    }
                    break;

                case SDL_TEXTINPUT:
                    if (!activeField.empty() && iniData.count(currentSection)) {
                        typedInput += event.text.text;
                        iniData[currentSection].keyValues[activeField] = typedInput;
                    }
                    break;

                case SDL_KEYDOWN:
                    if (!activeField.empty() && iniData.count(currentSection)) {
                        if (event.key.keysym.sym == SDLK_BACKSPACE && !typedInput.empty()) {
                            typedInput.pop_back();
                            iniData[currentSection].keyValues[activeField] = typedInput;
                        } else if (event.key.keysym.sym == SDLK_RETURN || event.key.keysym.sym == SDLK_ESCAPE) {
                            activeField.clear();
                            SDL_StopTextInput();
                        }
                    } else if (event.key.keysym.sym == SDLK_DOWN && iniData.count(currentSection)) {
                        scrollOffset += 20;
                        int maxScroll = static_cast<int>(iniData[currentSection].keyValues.size()) * 30 - 350;
                        if (scrollOffset > maxScroll) scrollOffset = maxScroll < 0 ? 0 : maxScroll;
                    } else if (event.key.keysym.sym == SDLK_UP) {
                        scrollOffset -= 20;
                        if (scrollOffset < 0) scrollOffset = 0;
                    }
                    break;
            }
        }
    }

    void render() {
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255); // White background
        SDL_RenderClear(renderer);

        // Render section dropdown
        renderText("Section: " + (currentSection.empty() ? "None" : currentSection), 10, 10);
        if (dropdownOpen) {
            int y = 40;
            for (size_t i = 0; i < sections.size(); ++i) {
                if (static_cast<int>(i) == dropdownHoverIndex) {
                    SDL_Rect highlight = {10, y, 190, 20};
                    SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255); // Highlight color
                    SDL_RenderFillRect(renderer, &highlight);
                }
                renderText(sections[i], 15, y); // Render section name
                y += 20;
            }
        }

        // Render key-value pairs
        int y = 50;
        if (iniData.count(currentSection)) { // Check if section exists
            for (const auto& [key, value] : iniData[currentSection].keyValues) {
                if (y + 20 - scrollOffset < 40 || y - scrollOffset > 400) { // Skip rendering if out of view
                    y += 30;
                    continue;
                }
                renderText(key, 10, y - scrollOffset); // Render key
                if (key == activeField) {
                    SDL_Rect fieldRect = {150, y - scrollOffset, 300, 20};
                    SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255); // Active field color
                    SDL_RenderFillRect(renderer, &fieldRect);
                }
                renderText(value, 150, y - scrollOffset); // Render value
                if (explanations.count(key)) renderText("?", 120, y - scrollOffset); // Render tooltip indicator
                y += 30;
            }
        }

        // Render Save/Exit buttons
        SDL_Rect saveBtn = {10, 360, 55, 25};
        SDL_Rect exitBtn = {75, 360, 55, 25};
        SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255); // Button color
        SDL_RenderFillRect(renderer, &saveBtn);
        SDL_RenderFillRect(renderer, &exitBtn);
        renderText("Save", 20, 365); // Render Save button text
        renderText("Exit", 90, 365); // Render Exit button text

        // Render tooltip
        if (!tooltipKey.empty() && explanations.count(tooltipKey)) {
            SDL_Rect tooltipRect = {150, 50, 300, 100};
            SDL_SetRenderDrawColor(renderer, 240, 240, 200, 255); // Tooltip background color
            SDL_RenderFillRect(renderer, &tooltipRect);
            std::string text = explanations[tooltipKey];
            int y = 55;
            size_t pos = 0;
            while (pos < text.length()) {
                size_t next = text.find('\n', pos);
                if (next == std::string::npos) next = text.length();
                std::string line = text.substr(pos, next - pos);
                renderText(line, 155, y); // Render tooltip text line by line
                y += 20;
                pos = next + 1;
            }
        }

        SDL_RenderPresent(renderer); // Present the rendered frame
    }

    void renderText(const std::string& text, int x, int y) {
        if (!font || text.empty()) return; // Check if font is loaded and text is not empty
        SDL_Color color = {0, 0, 0, 255}; // Black color for text
        SDL_Surface* surface = TTF_RenderText_Solid(font, text.c_str(), color); // Render text to surface
        if (!surface) {
            std::cerr << "TTF_RenderText_Solid failed: " << TTF_GetError() << std::endl; // Error handling
            return;
        }
        SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface); // Create texture from surface
        if (!texture) {
            std::cerr << "SDL_CreateTextureFromSurface failed: " << SDL_GetError() << std::endl; // Error handling
            SDL_FreeSurface(surface); // Free surface if texture creation fails
            return;
        }
        SDL_Rect dst = {x, y, surface->w, surface->h}; // Destination rectangle for rendering
        SDL_RenderCopy(renderer, texture, nullptr, &dst); // Copy texture to renderer
        SDL_FreeSurface(surface); // Free the surface
        SDL_DestroyTexture(texture); // Destroy the texture
    }
};

int main() {
    IniEditor editor("config.ini");
    editor.run();
    return 0;
}