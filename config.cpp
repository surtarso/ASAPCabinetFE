// main.cpp
// This example reimplements an INI settings editor using Dear ImGui.
// It uses SDL2 with an OpenGL3 context as the backend for rendering.
// Make sure you have SDL2, OpenGL, and Dear ImGui (plus imgui_impl_sdl and imgui_impl_opengl3)
// installed and configured in your build environment.
// sudo apt update && sudo apt install -y libsdl2-dev libgl1-mesa-dev libglu1-mesa-dev 
// libglew-dev libfreetype6-dev libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev
// g++ config.cpp imgui/*.cpp imgui/backends/imgui_impl_sdl2.cpp imgui/backends/imgui_impl_opengl3.cpp -std=c++17 -I/usr/include/SDL2 -D_REENTRANT -Iimgui -Iimgui/backends -lSDL2 -lGL -o config

#include <SDL.h>
#include <SDL_opengl.h>
#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_opengl3.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <map>
#include <vector>
#include <string>
#include <cstring>

// Structure representing a configuration section (a set of key-value pairs)
struct ConfigSection {
    std::map<std::string, std::string> keyValues;
    std::map<std::string, size_t> keyToLineIndex;
};

// IniEditor class handles loading, editing, and saving INI files via a GUI.
class IniEditor {
public:
    // Constructor loads the INI file and initializes key explanations.
    IniEditor(const std::string& filename);
    // Destructor.
    ~IniEditor();
    // Runs the main GUI loop.
    void run();

private:
    // Loads the INI file into memory.
    void loadIniFile(const std::string& filename);
    // Saves the current configuration back to the INI file.
    void saveIniFile(const std::string& filename);
    // Initializes tooltips/explanations for specific keys.
    void initExplanations();
    // Draws the ImGui GUI.
    void drawGUI();

    // Data members
    std::map<std::string, ConfigSection> iniData; // Map of section name to key-values.
    std::vector<std::string> sections;              // List of section names.
    std::map<std::string, std::string> explanations;  // Explanations for config keys.
    std::string currentSection;                     // Currently selected section.
    std::string iniFilename;                        // Name/path of the INI file.
    bool exitRequested = false;                     // Flag to exit the main loop.

    // Members to preserve comments and provide save feedback
    std::vector<std::string> originalLines;       // Stores the original file content
    std::map<size_t, std::pair<std::string, std::string>> lineToKey; // Maps line index to {section, key}
    bool showSavedMessage = false;                // Controls display of "Saved!" message
    double savedMessageTimer = 0.0;               // Tracks when the save message was triggered
};

// ---------------- Implementation ----------------

IniEditor::IniEditor(const std::string& filename) : iniFilename(filename) {
    loadIniFile(filename);
    initExplanations();
    if (!sections.empty()) {
        currentSection = sections[0];
    }
}

IniEditor::~IniEditor() {
    // No dynamic allocations to clean up in this example.
}

void IniEditor::loadIniFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()){
        std::cerr << "Could not open " << filename << std::endl;
        return;
    }

    // Read all lines into originalLines
    std::string line;
    while (std::getline(file, line)) {
        originalLines.push_back(line);
    }
    file.close();

    // Parse the lines to populate iniData, sections, and line mappings
    std::string currentSectionName;
    size_t lineIndex = 0;
    for (const auto& line : originalLines) {
        // Trim leading whitespace
        size_t start = line.find_first_not_of(" \t");
        if (start == std::string::npos) {
            lineIndex++;
            continue;
        }
        std::string trimmedLine = line.substr(start);

        // Skip empty lines or comments (but they’re preserved in originalLines)
        if (trimmedLine.empty() || trimmedLine[0] == ';') {
            lineIndex++;
            continue;
        }

        // Check for section header
        if (trimmedLine.front() == '[' && trimmedLine.back() == ']') {
            currentSectionName = trimmedLine.substr(1, trimmedLine.size() - 2);
            sections.push_back(currentSectionName);
            iniData[currentSectionName] = ConfigSection();
        } else if (!currentSectionName.empty()) {
            // Parse key=value pairs
            size_t pos = trimmedLine.find('=');
            if (pos != std::string::npos) {
                std::string key = trimmedLine.substr(0, pos);
                std::string value = trimmedLine.substr(pos + 1);
                // Trim trailing whitespace from key
                size_t endKey = key.find_last_not_of(" \t");
                if (endKey != std::string::npos)
                    key = key.substr(0, endKey + 1);
                // Trim leading whitespace from value
                size_t startValue = value.find_first_not_of(" \t");
                if (startValue != std::string::npos)
                    value = value.substr(startValue);
                iniData[currentSectionName].keyValues[key] = value;
                // Record the line index for this key
                iniData[currentSectionName].keyToLineIndex[key] = lineIndex;
                lineToKey[lineIndex] = {currentSectionName, key};
            }
        }
        lineIndex++;
    }
}

void IniEditor::saveIniFile(const std::string& filename) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Could not write " << filename << std::endl;
        return;
    }

    // Write back the original lines, updating only key-value pair lines
    for (size_t i = 0; i < originalLines.size(); ++i) {
        if (lineToKey.find(i) != lineToKey.end()) {
            // This line is a key-value pair; write the updated value
            auto [section, key] = lineToKey[i];
            file << key << " = " << iniData[section].keyValues[key] << "\n";
        } else {
            // This line is a comment, blank line, or section header; preserve it
            file << originalLines[i] << "\n";
        }
    }
    file.close();
}

void IniEditor::initExplanations() {
    explanations["TablesPath"] = "Specifies the absolute path to the folder containing VPX table files.\n1 - Must be a full path (e.g., /home/user/tables/).";
    explanations["ExecutableCmd"] = "Defines the absolute path to the VPinballX executable.";
    explanations["StartArgs"] = "Optional command-line arguments to prepend to the executable.";
    explanations["EndArgs"] = "Optional arguments to append after the table file in the command.";
    explanations["TableImage"] = "Relative path to the table's preview image.";
    explanations["BackglassImage"] = "Relative path to the backglass image.";
    explanations["WheelImage"] = "Relative path to the wheel image for the table.";
    explanations["DmdImage"] = "Relative path to the DMD or marquee image.";
    explanations["TableVideo"] = "Relative path to the table preview video.";
    explanations["BackglassVideo"] = "Relative path to the backglass video.";
    explanations["DmdVideo"] = "Relative path to the DMD video.";
    explanations["MainMonitor"] = "Index of the monitor for the table playfield window.";
    explanations["MainWidth"] = "Width of the main window in pixels.";
    explanations["MainHeight"] = "Height of the main window in pixels.";
    explanations["SecondMonitor"] = "Index of the monitor for the backglass/DMD window.";
    explanations["SecondWidth"] = "Width of the secondary window in pixels.";
    explanations["SecondHeight"] = "Height of the secondary window in pixels.";
    explanations["Path"] = "Absolute path to the font file used in the UI.";
    explanations["Size"] = "Font size in points for text rendering.";
    explanations["WheelImageSize"] = "Size of the wheel image in pixels.";
    explanations["WheelImageMargin"] = "Margin around the wheel image in pixels.";
    explanations["BackglassWidth"] = "Width of the backglass media in pixels.";
    explanations["BackglassHeight"] = "Height of the backglass media in pixels.";
    explanations["DmdWidth"] = "Width of the DMD media in pixels.";
    explanations["DmdHeight"] = "Height of the DMD media in pixels.";
}

void IniEditor::drawGUI() {
    // Set the window to cover the entire viewport
    ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize, ImGuiCond_Always);
    
    // Begin a full-screen window with no title bar, movement, or resizing
    ImGui::Begin("Ini Config Editor", nullptr, 
                 ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize);

    // Create a combo box for section selection
    if (ImGui::BeginCombo("Section", currentSection.c_str())) {
        for (const auto& section : sections) {
            bool is_selected = (currentSection == section);
            if (ImGui::Selectable(section.c_str(), is_selected))
                currentSection = section;
            if (is_selected)
                ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }

    // Calculate the height needed for the buttons, including spacing
    float buttonHeight = ImGui::GetFrameHeight() + ImGui::GetStyle().ItemSpacing.y;

    // Get the available height in the window from the current cursor position
    float availableHeight = ImGui::GetContentRegionAvail().y;

    // Set the child window height to available height minus button height
    float childHeight = availableHeight - buttonHeight;
    if (childHeight < 0) childHeight = 0; // Prevent negative height

    // Child region for scrolling key-value pairs
    ImGui::BeginChild("KeyValues", ImVec2(0, childHeight), true);
    if (iniData.find(currentSection) != iniData.end()) {
        for (auto& kv : iniData[currentSection].keyValues) {
            ImGui::PushID(kv.first.c_str());
            
            // Render the key
            ImGui::Text("%s", kv.first.c_str());
            
            // Position and render the [?] sign
            ImGui::SameLine(150);
            if (explanations.find(kv.first) != explanations.end()) {
                ImGui::TextColored(ImVec4(0, 1, 0, 1), "[?]");
                if (ImGui::IsItemHovered()) {
                    ImGui::BeginTooltip();
                    ImGui::Dummy(ImVec2(200.0f, 0.0f)); // Ensure tooltip has a minimum width
                    ImGui::TextWrapped("%s", explanations[kv.first].c_str());
                    ImGui::EndTooltip();
                }
            }
            
            // Position and render the value input field
            ImGui::SameLine(200);
            char buf[256];
            std::strncpy(buf, kv.second.c_str(), sizeof(buf));
            buf[sizeof(buf) - 1] = '\0';
            if (ImGui::InputText("", buf, sizeof(buf))) {
                kv.second = std::string(buf);
            }
            
            ImGui::PopID();
            ImGui::NewLine(); // Move to the next line for the next pair
        }
    } else {
        ImGui::Text("No section data available.");
    }
    ImGui::EndChild();

    // Save and Exit buttons
    if (ImGui::Button("Save")) {
        saveIniFile(iniFilename);
        showSavedMessage = true;
        savedMessageTimer = ImGui::GetTime();
    }
    ImGui::SameLine();
    if (ImGui::Button("Exit")) {
        exitRequested = true;
    }

    ImGui::SameLine();
    if (showSavedMessage) {
        ImGui::Text("Saved!");
        if (ImGui::GetTime() - savedMessageTimer > 2.0) {
            showSavedMessage = false;
        }
    }

    ImGui::End();
}

void IniEditor::run() {
    // Initialize SDL with video, timer, and game controller support.
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0) {
        std::cerr << "Error: " << SDL_GetError() << std::endl;
        return;
    }

    // Set OpenGL attributes and GLSL version.
    const char* glsl_version = "#version 130";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

    // Create an SDL window with an OpenGL context.
    SDL_Window* window = SDL_CreateWindow("INI Config Editor",
                                          SDL_WINDOWPOS_CENTERED,
                                          SDL_WINDOWPOS_CENTERED,
                                          800, 600,
                                          SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    SDL_GL_MakeCurrent(window, gl_context);
    SDL_GL_SetSwapInterval(1); // Enable vsync.

    // Setup Dear ImGui context.
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer backends.
    ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // Main loop.
    exitRequested = false;
    bool done = false;
    while (!done && !exitRequested) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT)
                done = true;
            if (event.type == SDL_WINDOWEVENT &&
                event.window.event == SDL_WINDOWEVENT_CLOSE &&
                event.window.windowID == SDL_GetWindowID(window))
                done = true;
        }

        // Start a new Dear ImGui frame.
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        // Draw the configuration editor GUI.
        drawGUI();

        // Rendering.
        ImGui::Render();
        glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
        glClearColor(0.45f, 0.55f, 0.60f, 1.00f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(window);
    }

    // Cleanup.
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

// ---------------- Main Entry Point ----------------

int main(int, char**) {
    // Create an IniEditor instance for "config.ini" and run the GUI.
    IniEditor editor("config.ini");
    editor.run();
    return 0;
}
