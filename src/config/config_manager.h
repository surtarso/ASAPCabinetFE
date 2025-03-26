#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include "config/settings.h"
#include <string>
#include <memory>
#include <vector>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

class AssetManager;
class Table;

class ConfigManager {
public:
    ConfigManager(const std::string& configPath);
    void loadConfig(); // Loads config from file
    void saveConfig(); // Saves config to file
    const Settings& getSettings() const; // Access settings
    void applyConfigChanges(SDL_Window* mainWindow, SDL_Window* playfieldWindow); // Apply settings to windows
    void notifyConfigChanged(AssetManager& assetManager, size_t& selectedTableIndex, std::vector<Table>& tables); // Reload assets

private:
    Settings settings; // All config data stored here
    std::string configPath_; // Path to config file

    // Helper to parse config file into settings
    void parseIniFile(const std::string& filename);
    // Helper to write settings back to file
    void writeIniFile(const std::string& filename);
};

#endif // CONFIG_MANAGER_H