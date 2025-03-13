// config.cpp
// As Simple As Possible Cabinet Front End - Configuration
// Reads and saves a local config.ini file while listing in sections with explanations.
// Tarso Galvão Mar/2025

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
    std::vector<std::pair<std::string, std::string>> keyValues; // Changed to vector to preserve order
    std::map<std::string, size_t> keyToLineIndex;               // Still using map for quick lookup
};

// --------------- IniEditor Class ------------------
class IniEditor {
public:
    IniEditor(const std::string& filename);           // Loads the INI file.
    ~IniEditor();                                     // Destructor.
    void run();                                       // Runs the main GUI loop.

private:
    void loadIniFile(const std::string& filename);    // Loads the INI file into memory.
    void saveIniFile(const std::string& filename);    // Saves the current configuration back to the INI file.
    void initExplanations();                          // Initializes tooltips.
    void drawGUI();                                   // Draws the ImGui GUI.

    // Data members
    std::map<std::string, ConfigSection> iniData;     // Map of section name to key-values.
    std::vector<std::string> sections;                // List of section names.
    std::map<std::string, std::string> explanations;  // Explanations for config keys.
    std::string currentSection;                       // Currently selected section.
    std::string iniFilename;                          // Name/path of the INI file.
    bool exitRequested = false;                       // Flag to exit the main loop.
    std::vector<std::string> originalLines;           // Stores the original file content
    std::map<size_t, std::pair<std::string, std::string>> lineToKey; // Maps line index to {section, key}
    bool showSavedMessage = false;                    // Controls display of "Saved!" message
    double savedMessageTimer = 0.0;                   // Tracks when the save message was triggered
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
    // No dynamic allocations
}

void IniEditor::loadIniFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
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
                // Append to vector to preserve order
                iniData[currentSectionName].keyValues.emplace_back(key, value);
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
            // Find the key in the vector (since keys might not be unique, use line index)
            for (const auto& kv : iniData[section].keyValues) {
                if (kv.first == key && iniData[section].keyToLineIndex[key] == i) {
                    file << key << " = " << kv.second << "\n";
                    break;
                }
            }
        } else {
            // This line is a comment, blank line, or section header; preserve it
            file << originalLines[i] << "\n";
        }
    }
    file.close();
}

void IniEditor::initExplanations() {
    explanations["TablesPath"] = "Specifies the absolute path to the folder containing VPX table files.\n1 - Must be a full path\n(e.g., /home/user/tables/).\nFinal command:\nStartArgs ExecutableCmd -play TablesPath/<selectedtable>.vpx EndArgs";
    explanations["ExecutableCmd"] = "Defines the absolute path to the VPinballX executable.\nFinal command:\nStartArgs ExecutableCmd -play TablesPath/<selectedtable>.vpx EndArgs";
    explanations["StartArgs"] = "Optional command-line arguments to prepend to the executable.\nFinal command:\nStartArgs ExecutableCmd -play TablesPath/<selectedtable>.vpx EndArgs";
    explanations["EndArgs"] = "Optional arguments to append after the table file in the command.\nFinal command:\nStartArgs ExecutableCmd -play TablesPath/<selectedtable>.vpx EndArgs";
    explanations["TableImage"] = "Relative path to the table's preview image.\nThese are relative to your table folder.\n(e.g., /path/to/tables/<table_folder>/).";
    explanations["BackglassImage"] = "Relative path to the backglass image.\nThese are relative to your table folder.\n(e.g., /path/to/tables/<table_folder>/).";
    explanations["WheelImage"] = "Relative path to the wheel image for the table.\nThese are relative to your table folder.\n(e.g., /path/to/tables/<table_folder>/).";
    explanations["DmdImage"] = "Relative path to the DMD or marquee image.\nThese are relative to your table folder.\n(e.g., /path/to/tables/<table_folder>/).";
    explanations["TableVideo"] = "Relative path to the table preview video.\nThese are relative to your table folder.\n(e.g., /path/to/tables/<table_folder>/).";
    explanations["BackglassVideo"] = "Relative path to the backglass video.\nThese are relative to your table folder.\n(e.g., /path/to/tables/<table_folder>/).";
    explanations["DmdVideo"] = "Relative path to the DMD video.\nThese are relative to your table folder.\n(e.g., /path/to/tables/<table_folder>/).";
    explanations["MainMonitor"] = "Index of the monitor for the table playfield window.\nYou can use 'xrandr' to get yours.";
    explanations["MainWidth"] = "Width of the main window in pixels.\nThis should be relative to your playfield media width.";
    explanations["MainHeight"] = "Height of the main window in pixels.\nThis should be relative to your playfield media height.";
    explanations["SecondMonitor"] = "Index of the monitor for the backglass/DMD window.\nYou can use 'xrandr' to get yours.";
    explanations["SecondWidth"] = "Width of the secondary window in pixels.\nThis should be relative to your backglass + DMD media widht.";
    explanations["SecondHeight"] = "Height of the secondary window in pixels.\nThis should be relative to your backglass + DMD media height.";
    explanations["Path"] = "Absolute path to the font file used in the UI.";
    explanations["Size"] = "Font size in points for table title text rendering.";
    explanations["WheelImageSize"] = "Size of the wheel image in pixels.\nThis considers a square image.";
    explanations["WheelImageMargin"] = "Margin around the wheel image in pixels.";
    explanations["BackglassWidth"] = "Width of the backglass media in pixels.";
    explanations["BackglassHeight"] = "Height of the backglass media in pixels.";
    explanations["DmdWidth"] = "Width of the DMD media in pixels.";
    explanations["DmdHeight"] = "Height of the DMD media in pixels.";
    explanations["FadeTargetAlpha"] = "Goes from 0 (transparent) to 255.\nUse 128 for ~50 percent alpha";
    explanations["FadeDurationMs"] = "Table images switch transition time in ms\nSet to 1 if using videos.";
}

void IniEditor::drawGUI() {
    ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize, ImGuiCond_Always);
    
    ImGui::Begin("ASAPCabinetFE Configuration", nullptr, 
                 ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize);

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

    float buttonHeight = ImGui::GetFrameHeight() + ImGui::GetStyle().ItemSpacing.y;
    float availableHeight = ImGui::GetContentRegionAvail().y;
    float childHeight = availableHeight - buttonHeight;
    if (childHeight < 0) childHeight = 0;

    ImGui::BeginChild("KeyValues", ImVec2(0, childHeight), true);
    if (iniData.find(currentSection) != iniData.end()) {
        for (auto& kv : iniData[currentSection].keyValues) { // Iterate over vector instead of map
            ImGui::PushID(kv.first.c_str());
            
            ImGui::Text("%s", kv.first.c_str());
            
            ImGui::SameLine(150);
            if (explanations.find(kv.first) != explanations.end()) {
                ImGui::TextColored(ImVec4(0, 1, 0, 1), "[?]");
                if (ImGui::IsItemHovered()) {
                    ImGui::BeginTooltip();
                    ImGui::Dummy(ImVec2(300.0f, 0.0f));
                    ImGui::TextWrapped("%s", explanations[kv.first].c_str());
                    ImGui::EndTooltip();
                }
            }
            
            ImGui::SameLine(200);
            char buf[256];
            std::strncpy(buf, kv.second.c_str(), sizeof(buf));
            buf[sizeof(buf) - 1] = '\0';
            if (ImGui::InputText("", buf, sizeof(buf))) {
                kv.second = std::string(buf); // Update the value directly in the vector
            }
            
            ImGui::PopID();
            ImGui::NewLine();
        }
    } else {
        ImGui::Text("No section data available.");
    }
    ImGui::EndChild();

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
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0) {
        std::cerr << "Error: " << SDL_GetError() << std::endl;
        return;
    }

    const char* glsl_version = "#version 130";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

    SDL_Window* window = SDL_CreateWindow("ASAPCabinetFE Configuration",
                                          SDL_WINDOWPOS_CENTERED,
                                          SDL_WINDOWPOS_CENTERED,
                                          800, 600,
                                          SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    SDL_GL_MakeCurrent(window, gl_context);
    SDL_GL_SetSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();

    ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL3_Init(glsl_version);

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

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        drawGUI();

        ImGui::Render();
        glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
        glClearColor(0.45f, 0.55f, 0.60f, 1.00f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(window);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

// ---------------- Main Entry Point ----------------

int main(int, char**) {
    IniEditor editor("config.ini");
    editor.run();
    return 0;
}