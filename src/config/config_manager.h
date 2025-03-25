#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include "config/settings.h"
#include "config/config_loader.h"
#include <string>
#include <memory>
#include <vector>
#include <cstddef>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

class AssetManager;
class Table;

class ConfigManager {
public:
    ConfigManager(const std::string& configPath, SDL_Window* mainWindow,
                  SDL_Renderer* mainRenderer, SDL_Window* playfieldWindow,
                  SDL_Renderer* playfieldRenderer, std::unique_ptr<TTF_Font, void(*)(TTF_Font*)>& font,
                  AssetManager& assetManager, size_t& selectedTableIndex,
                  std::vector<Table>& tables);
    void loadConfig(const std::string& configFile);
    const Settings& getSettings() const;
    void notifyConfigChanged();
    void applyConfigChanges();

private:
    Settings settings;
    std::string configPath_;
    SDL_Window* mainWindow_;
    SDL_Renderer* mainRenderer_;
    SDL_Window* playfieldWindow_;
    SDL_Renderer* playfieldRenderer_;
    std::unique_ptr<TTF_Font, void(*)(TTF_Font*)>& font_;
    AssetManager& assetManager_;
    size_t& selectedTableIndex_;
    std::vector<Table>& tables_;
};

#endif // CONFIG_MANAGER_H