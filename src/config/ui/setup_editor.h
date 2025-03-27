#ifndef SETUP_EDITOR_H
#define SETUP_EDITOR_H

#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include "SDL.h"
#include "config/settings_manager.h"
#include "keybinds/ikeybind_provider.h"

// Forward declarations
class AssetManager;
class TableLoader;

// --- Data Structures ---
struct SettingsSection {
    std::vector<std::pair<std::string, std::string>> keyValues; // Preserve order
    std::unordered_map<std::string, size_t> keyToLineIndex;     // Map key to line index
};

// --- GUI Utility Functions ---
namespace SettingsGuiUtils {
    void DrawSectionsPane(const std::vector<std::string>& sections, std::string& currentSection, 
                          bool& isCapturingKey, std::string& capturingKeyName, std::string& capturedKeyName);
    void DrawKeyValuesPane(SettingsSection& section, IKeybindProvider* keybindProvider, 
                           bool& isCapturingKey, std::string& capturingKeyName, std::string& capturedKeyName, 
                           const std::string& currentSection, std::map<std::string, bool>& showPicker, 
                           const std::unordered_map<std::string, std::string>& explanations, bool& hasChanges);
    void DrawButtonPane(bool& showFlag, const std::string& iniFilename, SettingsManager* configManager, 
                        AssetManager* assets, size_t* currentIndex, std::vector<TableLoader>* tables, 
                        bool& hasChanges, bool& isCapturingKey, std::string& capturingKeyName, 
                        std::string& capturedKeyName, float& saveMessageTimer, 
                        const std::map<std::string, SettingsSection>& iniData);
    void ParseColorString(const std::string& colorStr, float color[4]);
    std::string ColorToString(const float color[4]);
}

// --- Base Config Editor Class ---
class ConfigEditor {
public:
    ConfigEditor(const std::string& filename, bool& showFlag, SettingsManager* configManager, 
                 IKeybindProvider* keybindProvider);
    virtual ~ConfigEditor() = default;

    void drawGUI();
    void handleEvent(const SDL_Event& event);
    bool isCapturingKey() const { return isCapturingKey_; }
    void setFillParentWindow(bool fill) { fillParentWindow_ = fill; }

    Settings tempSettings_; // Public for app access

protected:
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
    std::map<std::string, bool> showPicker_;

    static const std::vector<std::string> sectionOrder_;

    virtual void loadIniFile(const std::string& filename);
    void initExplanations();
    virtual AssetManager* getAssets() { return nullptr; }
    virtual size_t* getCurrentIndex() { return nullptr; }
    virtual std::vector<TableLoader>* getTables() { return nullptr; }
    virtual void drawTableOverridesGUI() {}
};

// --- Setup Editor for Invalid Config Popup ---
class SetupEditor : public ConfigEditor {
public:
    SetupEditor(const std::string& filename, bool& showFlag, SettingsManager* configManager, 
                IKeybindProvider* keybindProvider)
        : ConfigEditor(filename, showFlag, configManager, keybindProvider) {}
};

// --- Runtime Editor for In-App Config Menu ---
class RuntimeEditor : public ConfigEditor {
public:
    RuntimeEditor(const std::string& filename, bool& showFlag, SettingsManager* configManager, 
                  IKeybindProvider* keybindProvider, AssetManager* assets, size_t* currentIndex, 
                  std::vector<TableLoader>* tables);

protected:
    AssetManager* assets_;
    size_t* currentIndex_;
    std::vector<TableLoader>* tables_;

    AssetManager* getAssets() override { return assets_; }
    size_t* getCurrentIndex() override { return currentIndex_; }
    std::vector<TableLoader>* getTables() override { return tables_; }
    void drawTableOverridesGUI() override;
};

#endif // SETUP_EDITOR_H