#ifndef SETTINGS_MANAGER_H
#define SETTINGS_MANAGER_H

#include "config/settings.h"
#include "keybinds/keybind_manager.h"
#include <string>
#include <memory>
#include <vector>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

class AssetManager;
class TableLoader;

class SettingsManager {
public:
    SettingsManager(const std::string& configPath);
    void loadConfig(); // Loads config from file
    void saveConfig(); // Saves config to file
    const Settings& getSettings() const; // Access settings
    KeybindManager& getKeybindManager(); // Access keybind manager
    const KeybindManager& getKeybindManager() const; // Const access
    void applyConfigChanges(SDL_Window* mainWindow, SDL_Window* playfieldWindow); // Apply settings to windows
    void notifyConfigChanged(AssetManager& assetManager, size_t& selectedTableIndex, std::vector<TableLoader>& tables); // Reload assets

private:
    Settings settings; // All config data stored here (except keybinds)
    KeybindManager keybindManager_; // Manages keybindings
    std::string configPath_; // Path to config file

    // Helper to parse config file into settings
    void parseIniFile(const std::string& filename);
    // Helper to write settings back to file
    void writeIniFile(const std::string& filename);
};

#endif // SETTINGS_MANAGER_H