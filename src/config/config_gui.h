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

// Common utility functions for drawing shared UI elements
namespace ConfigGuiUtils {
    void DrawSectionsPane(const std::vector<std::string>& sections, std::string& currentSection, bool& isCapturingKey, std::string& capturingKeyName, std::string& capturedKeyName);
    void DrawKeyValuesPane(ConfigSection& section, IKeybindProvider* keybindProvider, bool& isCapturingKey, std::string& capturingKeyName, std::string& capturedKeyName, const std::string& currentSection, std::map<std::string, bool>& showPicker, const std::unordered_map<std::string, std::string>& explanations, bool& hasChanges);
    void DrawButtonPane(bool& showFlag, const std::string& iniFilename, ConfigManager* configManager, AssetManager* assets, size_t* currentIndex, std::vector<Table>* tables, bool& hasChanges, bool& isCapturingKey, std::string& capturingKeyName, std::string& capturedKeyName, float& saveMessageTimer, const std::map<std::string, ConfigSection>& iniData);
    void ParseColorString(const std::string& colorStr, float color[4]);
    std::string ColorToString(const float color[4]);
}

class InitialConfigEditor {
public:
    InitialConfigEditor(const std::string& filename, bool& showFlag, ConfigManager* configManager,
                        IKeybindProvider* keybindProvider);
    ~InitialConfigEditor() = default;

    void drawGUI();
    void handleEvent(const SDL_Event& event);
    bool isCapturingKey() const { return isCapturingKey_; }
    void setFillParentWindow(bool fill) { fillParentWindow_ = fill; }

    Settings tempSettings_; // Public for App to access

private:
    std::string iniFilename_;
    bool& showFlag_;
    ConfigManager* configManager_;
    IKeybindProvider* keybindProvider_;
    std::vector<std::string> originalLines_;
    std::map<std::string, ConfigSection> iniData_;
    std::vector<std::string> sections_;
    std::string currentSection_;
    std::unordered_map<size_t, std::pair<std::string, std::string>> lineToKey_;
    std::unordered_map<std::string, std::string> explanations_;
    bool hasChanges_ = false;
    bool isCapturingKey_ = false;
    std::string capturingKeyName_;
    std::string capturedKeyName_;
    bool fillParentWindow_ = false;
    float saveMessageTimer_ = 0.0f;
    std::map<std::string, bool> showPicker_; // Tracks if color picker is visible for each key

    void loadIniFile(const std::string& filename);
    void saveIniFile(const std::string& filename);
    void initExplanations();
};

class InGameConfigEditor {
public:
    InGameConfigEditor(const std::string& filename, bool& showFlag, ConfigManager* configManager,
                       IKeybindProvider* keybindProvider, AssetManager* assets,
                       size_t* currentIndex, std::vector<Table>* tables);
    ~InGameConfigEditor() = default;

    void drawGUI();
    void handleEvent(const SDL_Event& event);
    bool isCapturingKey() const { return isCapturingKey_; }

    Settings tempSettings_; // Public for App to access

private:
    std::string iniFilename_;
    bool& showFlag_;
    ConfigManager* configManager_;
    IKeybindProvider* keybindProvider_;
    AssetManager* assets_;
    size_t* currentIndex_;
    std::vector<Table>* tables_;
    std::vector<std::string> originalLines_;
    std::map<std::string, ConfigSection> iniData_;
    std::vector<std::string> sections_;
    std::string currentSection_;
    std::unordered_map<size_t, std::pair<std::string, std::string>> lineToKey_;
    std::unordered_map<std::string, std::string> explanations_;
    bool hasChanges_ = false;
    bool isCapturingKey_ = false;
    std::string capturingKeyName_;
    std::string capturedKeyName_;
    float saveMessageTimer_ = 0.0f;
    std::map<std::string, bool> showPicker_; // Tracks if color picker is visible for each key

    void loadIniFile(const std::string& filename);
    void saveIniFile(const std::string& filename);
    void initExplanations();
    void drawTableOverridesGUI();
};

#endif // CONFIG_GUI_H