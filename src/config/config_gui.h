#ifndef CONFIG_GUI_H
#define CONFIG_GUI_H

#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include "SDL.h"

// Declare configChangesPending as extern so it can be accessed in main.cpp
extern bool configChangesPending;

struct ConfigSection {
    std::vector<std::pair<std::string, std::string>> keyValues; // Preserve order
    std::unordered_map<std::string, size_t> keyToLineIndex;     // Map key to its line index
};

class IniEditor {
public:
    IniEditor(const std::string& filename, bool& showFlag);
    ~IniEditor();
    void drawGUI();
    void handleEvent(const SDL_Event& event);

    // Public getter for isCapturingKey_
    bool isCapturingKey() const { return isCapturingKey_; }

private:
    std::string iniFilename;
    bool& showFlag;
    std::vector<std::string> originalLines;
    std::map<std::string, ConfigSection> iniData;
    std::vector<std::string> sections;
    std::string currentSection;
    std::unordered_map<size_t, std::pair<std::string, std::string>> lineToKey;
    std::unordered_map<std::string, std::string> explanations;
    bool hasChanges = false;

    // Key capture state
    bool isCapturingKey_ = false;
    std::string capturingKeyName_;
    std::string capturedKeyName_;

    // Save message timer
    float saveMessageTimer_ = 0.0f; // Timer for displaying "Saved successfully"

    void loadIniFile(const std::string& filename);
    void saveIniFile(const std::string& filename);
    void initExplanations();
};

#endif // CONFIG_GUI_H