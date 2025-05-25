/**
 * @file iasset_manager.h
 * @brief Defines the IAssetManager interface for managing assets in ASAPCabinetFE.
 *
 * This interface abstracts asset management for textures, video players, fonts, and
 * audio, providing methods to load, access, and update assets for Visual Pinball X
 * (VPX) tables. Implementations ensure proper resource management and coordination
 * with rendering and sound systems.
 */
#ifndef IASSET_MANAGER_H
#define IASSET_MANAGER_H

#include <SDL.h>
#include <SDL_ttf.h>
#include <memory>
#include <string>
#include <vector>
#include "config/iconfig_service.h"
#include "render/table_loader.h"
#include "core/iwindow_manager.h"
#include "render/ivideo_player.h"

/**
 * @class ISoundManager
 * @brief Interface for sound management (forward declaration).
 */
class ISoundManager;

/**
 * @brief The IAssetManager interface provides methods to manage and access various assets used in the application.
 *
 * This interface abstracts asset management functionality including handling textures, video players,
 * configuration settings, fonts, audio, and positions for display elements. Implementations of this interface should
 * ensure proper resource management and provide mechanisms for updating assets dynamically.
 */
class IAssetManager {
public:
    /**
     * @brief Virtual destructor to ensure derived classes can clean up properly.
     */
    virtual ~IAssetManager() = default;
    
    // Texture accessors
    /**
     * @brief Retrieves the texture used for the playfield.
     * @return SDL_Texture* Pointer to the playfield texture.
     */
    virtual SDL_Texture* getPlayfieldTexture() = 0;

    /**
     * @brief Retrieves the texture used for the wheel.
     * @return SDL_Texture* Pointer to the wheel texture.
     */
    virtual SDL_Texture* getWheelTexture() = 0;

    /**
     * @brief Retrieves the texture used for the backglass.
     * @return SDL_Texture* Pointer to the backglass texture.
     */
    virtual SDL_Texture* getBackglassTexture() = 0;

    /**
     * @brief Retrieves the texture used for the dot matrix display (DMD).
     * @return SDL_Texture* Pointer to the DMD texture.
     */
    virtual SDL_Texture* getDmdTexture() = 0;

    /**
     * @brief Retrieves the texture used for the title.
     * @return SDL_Texture* Pointer to the title texture.
     */
    virtual SDL_Texture* getTitleTexture() = 0;
    
    // Video player accessors
    /**
     * @brief Retrieves the video player associated with the playfield.
     * @return IVideoPlayer* Pointer to the playfield video player.
     * @see IVideoPlayer
     */
    virtual IVideoPlayer* getPlayfieldVideoPlayer() = 0;

    /**
     * @brief Retrieves the video player associated with the backglass.
     * @return IVideoPlayer* Pointer to the backglass video player.
     * @see IVideoPlayer
     */
    virtual IVideoPlayer* getBackglassVideoPlayer() = 0;

    /**
     * @brief Retrieves the video player associated with the dot matrix display (DMD).
     * @return IVideoPlayer* Pointer to the DMD video player.
     * @see IVideoPlayer
     */
    virtual IVideoPlayer* getDmdVideoPlayer() = 0;
    
    // Settings and positioning
    /**
     * @brief Retrieves the configuration service that manages application settings.
     * @return IConfigService* Pointer to the settings manager.
     * @see IConfigService
     */
    virtual IConfigService* getSettingsManager() = 0;

    /**
     * @brief Retrieves the rectangle that specifies the title's position and dimensions (x, y, width, height).
     * @return SDL_Rect The rectangle representing the title's size and position.
     */
    virtual SDL_Rect getTitleRect() = 0;

    /**
     * @brief Sets the title's position on the display.
     * @param x The new x-coordinate for the title.
     * @param y The new y-coordinate for the title.
     */
    virtual void setTitlePosition(int x, int y) = 0;
    
    // Font management
    /**
     * @brief Sets the current font for text rendering.
     * @param font Pointer to the TTF_Font object to be set.
     */
    virtual void setFont(TTF_Font* font) = 0;

    /**
     * @brief Reloads the title texture using a new title string and color, updating the title rectangle accordingly.
     * @param title The new title string.
     * @param color The color used for text rendering.
     * @param titleRect [in,out] Parameter that is updated to reflect the new title's dimensions (width and height) after the texture is reloaded.
     */
    virtual void reloadTitleTexture(const std::string& title, SDL_Color color, SDL_Rect& titleRect) = 0;

    // Asset management
    /**
     * @brief Reloads all relevant assets with updated settings, fonts, and table data.
     * @param windowManager Pointer to the window manager used for rendering.
     * @param font Pointer to the TTF_Font used for text rendering.
     * @param tables A vector containing table data necessary for asset creation.
     * @param index The index of the table data to reload assets for.
     * @see IWindowManager
     */
    virtual void reloadAssets(IWindowManager* windowManager, TTF_Font* font, const std::vector<TableData>& tables, size_t index) = 0;
    
    /**
     * @brief Sets the configuration service (settings manager) responsible for application settings.
     * @param cm Pointer to the new IConfigService instance.
     * @see IConfigService
     */
    virtual void setSettingsManager(IConfigService* cm) = 0;
    
    /**
     * @brief Loads assets for a specific table based on its index and associated table data.
     * @param index The index of the table to load assets for.
     * @param tables A vector containing table data.
     */
    virtual void loadTableAssets(size_t index, const std::vector<TableData>& tables) = 0;
    
    /**
     * @brief Clears any existing video players to free resources before initializing new ones.
     */
    virtual void clearOldVideoPlayers() = 0;
    
    /**
     * @brief Cleans up video players, releasing allocated resources appropriately.
     */
    virtual void cleanupVideoPlayers() = 0;

    /**
     * @brief Sets the sound manager for audio playback.
     * @param soundManager Pointer to the ISoundManager instance.
     * @see ISoundManager
     */
    virtual void setSoundManager(ISoundManager* soundManager) = 0;

    /**
     * @brief Plays table-specific music.
     * @param index The index of the table to play music for.
     * @param tables A vector containing table data with music paths.
     */
    virtual void playTableMusic(size_t index, const std::vector<TableData>& tables) = 0;

    /**
     * @brief Applies audio settings to all active video players.
     *
     * Updates the volume and mute state of playfield, backglass, and DMD video players
     * based on the current mediaAudioVol and mediaAudioMute settings.
     */
    virtual void applyVideoAudioSettings() = 0;
};

#endif // IASSET_MANAGER_H