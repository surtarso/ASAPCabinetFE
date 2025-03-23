#ifndef CONFIG_GUI_H
#define CONFIG_GUI_H

#include "imgui.h"
#include <SDL.h>
#include <string>
#include <map>
#include <unordered_map>
#include <vector>

struct ConfigSection {
    std::vector<std::pair<std::string, std::string>> keyValues;
    std::map<std::string, size_t> keyToLineIndex;
};

class IniEditor {
public:
    IniEditor(const std::string& filename, bool& showFlag);
    ~IniEditor();
    void drawGUI();
    void handleEvent(const SDL_Event& event);
    bool hasChangesMade() const { return hasChanges; }
    void resetChanges() { hasChanges = false; }

private:
    void loadIniFile(const std::string& filename);
    void saveIniFile(const std::string& filename);
    void initExplanations();

    std::map<std::string, ConfigSection> iniData;
    std::vector<std::string> sections;
    std::unordered_map<std::string, std::string> explanations;
    std::string currentSection;
    std::string iniFilename;
    bool& showFlag;
    std::vector<std::string> originalLines;
    std::map<size_t, std::pair<std::string, std::string>> lineToKey;
    bool hasChanges = false;  // Track if changes were made
};

// Declare the global configChangesPending variable
extern bool configChangesPending;

#endif // CONFIG_GUI_H