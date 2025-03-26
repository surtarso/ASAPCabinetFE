#ifndef CONFIG_GUI_H
#define CONFIG_GUI_H

#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include "SDL.h"
#include "config_manager.h"
#include "keybinds/ikeybind_provider.h"

struct ConfigSection {
    std::vector<std::pair<std::string, std::string>> keyValues; // Preserve order
    std::unordered_map<std::string, size_t> keyToLineIndex;     // Map key to its line index
};

class AssetManager; // Forward declaration
class Table;        // Forward declaration

class IniEditor {
public:
    // Main constructor for full app
    IniEditor(const std::string& filename, bool& showFlag, ConfigManager* configManager,
              IKeybindProvider* keybindProvider, AssetManager* assets = nullptr,
              size_t* currentIndex = nullptr, std::vector<Table>* tables = nullptr);
    ~IniEditor();
    void drawGUI();
    void handleEvent(const SDL_Event& event);

    bool isCapturingKey() const { return isCapturingKey_; }
    Settings tempSettings_; // Public for App to access

private:
    std::string iniFilename;
    bool& showFlag;
    ConfigManager* configManager_;
    IKeybindProvider* keybindProvider_; // For keybind management
    AssetManager* assets_;           // Pointer to App's assets (optional)
    size_t* currentIndex_;           // Pointer to App's currentIndex (optional)
    std::vector<Table>* tables_;     // Pointer to App's tables (optional)
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
    float saveMessageTimer_ = 0.0f;

    // Color picker visibility tracker
    std::map<std::string, bool> showPicker;  // Tracks if color picker is visible for each key

    // Helper functions for color editing
    void parseColorString(const std::string& colorStr, float color[4]);
    std::string colorToString(const float color[4]);

    void loadIniFile(const std::string& filename);
    void saveIniFile(const std::string& filename);
    void initExplanations();
};

#endif // CONFIG_GUI_H