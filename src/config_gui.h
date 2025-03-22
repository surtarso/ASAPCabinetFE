#ifndef CONFIG_GUI_H
#define CONFIG_GUI_H

#include "imgui.h"
#include <SDL.h>
#include <string>
#include <map>
#include <vector>

struct ConfigSection {
    std::vector<std::pair<std::string, std::string>> keyValues;
    std::map<std::string, size_t> keyToLineIndex;
};

class IniEditor {
public:
    IniEditor(const std::string& filename, bool& showFlag);
    ~IniEditor();  // Added destructor declaration
    void drawGUI();
    void handleEvent(const SDL_Event& event);

private:
    void loadIniFile(const std::string& filename);
    void saveIniFile(const std::string& filename);
    void initExplanations();

    std::map<std::string, ConfigSection> iniData;
    std::vector<std::string> sections;
    std::map<std::string, std::string> explanations;
    std::string currentSection;
    std::string iniFilename;
    bool& showFlag;
    std::vector<std::string> originalLines;
    std::map<size_t, std::pair<std::string, std::string>> lineToKey;
    bool showSavedMessage = false;
    double savedMessageTimer = 0.0;
};

#endif // CONFIG_GUI_H