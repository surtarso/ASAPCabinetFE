#ifndef SETUP_EDITOR_H
#define SETUP_EDITOR_H

#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include "SDL.h"
#include "config/settings_manager.h"
#include "keybinds/ikeybind_provider.h"

struct SettingsSection {
    std::vector<std::pair<std::string, std::string>> keyValues; // Preserve order
    std::unordered_map<std::string, size_t> keyToLineIndex;     // Map key to its line index
};

class AssetManager; // Forward declaration
class TableLoader;        // Forward declaration

// Common utility functions for drawing shared UI elements
namespace SettingsGuiUtils {
    void DrawSectionsPane(const std::vector<std::string>& sections, std::string& currentSection, bool& isCapturingKey, std::string& capturingKeyName, std::string& capturedKeyName);
    void DrawKeyValuesPane(SettingsSection& section, IKeybindProvider* keybindProvider, bool& isCapturingKey, std::string& capturingKeyName, std::string& capturedKeyName, const std::string& currentSection, std::map<std::string, bool>& showPicker, const std::unordered_map<std::string, std::string>& explanations, bool& hasChanges);
    void DrawButtonPane(bool& showFlag, const std::string& iniFilename, SettingsManager* configManager, AssetManager* assets, size_t* currentIndex, std::vector<TableLoader>* tables, bool& hasChanges, bool& isCapturingKey, std::string& capturingKeyName, std::string& capturedKeyName, float& saveMessageTimer, const std::map<std::string, SettingsSection>& iniData);
    void ParseColorString(const std::string& colorStr, float color[4]);
    std::string ColorToString(const float color[4]);
}

class SetupEditor {
public:
    SetupEditor(const std::string& filename, bool& showFlag, SettingsManager* configManager,
                        IKeybindProvider* keybindProvider);
    ~SetupEditor() = default;

    void drawGUI();
    void handleEvent(const SDL_Event& event);
    bool isCapturingKey() const { return isCapturingKey_; }
    void setFillParentWindow(bool fill) { fillParentWindow_ = fill; }

    Settings tempSettings_; // Public for App to access

private:
    // Reordered to match constructor initialization list:
    // iniFilename_, showFlag_, configManager_, keybindProvider_, tempSettings_
    std::string iniFilename_;
    bool& showFlag_;
    SettingsManager* configManager_;
    IKeybindProvider* keybindProvider_;
    std::vector<std::string> originalLines_;
    std::map<std::string, SettingsSection> iniData_;
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

    // Static list of section names in desired order (excluding "Table Overrides")
    static const std::vector<std::string> sectionOrder_;

    void loadIniFile(const std::string& filename);
    void saveIniFile(const std::string& filename);
    void initExplanations();
};

class RuntimeEditor {
public:
    RuntimeEditor(const std::string& filename, bool& showFlag, SettingsManager* configManager,
                       IKeybindProvider* keybindProvider, AssetManager* assets,
                       size_t* currentIndex, std::vector<TableLoader>* tables);
    ~RuntimeEditor() = default;

    void drawGUI();
    void handleEvent(const SDL_Event& event);
    bool isCapturingKey() const { return isCapturingKey_; }

    Settings tempSettings_; // Public for App to access

private:
    // Reordered to match constructor initialization list:
    // iniFilename_, showFlag_, configManager_, keybindProvider_, assets_, currentIndex_, tables_, tempSettings_
    std::string iniFilename_;
    bool& showFlag_;
    SettingsManager* configManager_;
    IKeybindProvider* keybindProvider_;
    AssetManager* assets_;
    size_t* currentIndex_;
    std::vector<TableLoader>* tables_;
    std::vector<std::string> originalLines_;
    std::map<std::string, SettingsSection> iniData_;
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

    // Static list of section names in desired order (excluding "Table Overrides")
    static const std::vector<std::string> sectionOrder_;

    void loadIniFile(const std::string& filename);
    void saveIniFile(const std::string& filename);
    void initExplanations();
    void drawTableOverridesGUI();
};

#endif // CONFIG_GUI_H