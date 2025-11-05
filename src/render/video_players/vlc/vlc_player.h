#ifndef VLC_PLAYER_H
#define VLC_PLAYER_H

#include "render/ivideo_player.h"
#include <SDL2/SDL.h>
#include <vlc/vlc.h>
#include <string>

/**
 * @brief Concrete implementation of IVideoPlayer using VLC (libVLC).
 *
 * This class provides video playback capabilities by integrating with the libVLC library.
 * It handles video decoding, rendering to an SDL_Texture, and basic playback controls,
 * including volume and mute for the video's audio track.
 */
class VlcVideoPlayer : public IVideoPlayer {
public:
    /**
     * @brief Constructs a VlcVideoPlayer instance.
     */
    VlcVideoPlayer();

    /**
     * @brief Destroys the VlcVideoPlayer instance and cleans up VLC resources.
     */
    ~VlcVideoPlayer() override;

    /**
     * @brief Sets up the VLC video player with the given renderer, path, and dimensions.
     * @param renderer Pointer to the SDL_Renderer.
     * @param path Path to the video file.
     * @param width Desired width of the video texture.
     * @param height Desired height of the video texture.
     * @return True if setup is successful, false otherwise.
     */
    bool setup(SDL_Renderer* renderer, const std::string& path, int width, int height) override;

    /**
     * @brief Starts video playback.
     */
    void play() override;

    /**
     * @brief Stops video playback.
     */
    void stop() override;

    /**
     * @brief Updates the video frame and texture.
     */
    void update() override;

    /**
     * @brief Retrieves the current video frame as an SDL_Texture.
     * @return Pointer to the SDL_Texture.
     */
    SDL_Texture* getTexture() const override;

    /**
     * @brief Returns the number of frames received from VLC (diagnostic).
     */
    int getFrameCount() const;


    /**
     * @brief Checks if the video is currently playing.
     * @return True if playing, false otherwise.
     */
    bool isPlaying() const override;

    /**
     * @brief Sets the volume for the video's audio track.
     * @param volume The desired volume, a float between 0.0 (silent) and 1.0 (full volume).
     */
    void setVolume(float volume) override;

    /**
     * @brief Sets the mute state for the video's audio track.
     * @param mute True to mute, false to unmute.
     */
    void setMute(bool mute) override;

private:
    /**
     * @brief Internal structure to hold VLC and SDL context for video playback.
     */
    struct VideoContext {
        libvlc_instance_t* instance;
        libvlc_media_player_t* player;
        SDL_Renderer* renderer;
        SDL_Texture* texture;
        void* pixels;
        int pitch;
        int width;
        int height;
        SDL_mutex* mutex;
        bool isPlaying;
        bool frameReady; // New: Flag to indicate if a new frame is ready for texture update
        int frameCount;  // Diagnostic: count frames received from VLC
    };

    VideoContext* ctx_; ///< Pointer to the internal video context.


    /**
     * @brief VLC callback for locking the video buffer.
     * @param data User data (VideoContext*).
     * @param pixels Pointer to the pixel buffer.
     * @return A pointer to the locked buffer.
     */
    static void* lock(void* data, void** pixels);

    /**
     * @brief VLC callback for unlocking the video buffer.
     * @param data User data (VideoContext*).
     * @param id Buffer ID.
     * @param pixels Pointer to the pixel buffer.
     */
    static void unlock(void* data, void* id, void* const* pixels);

    /**
     * @brief VLC callback for displaying the video frame.
     * @param data User data (VideoContext*).
     * @param id Buffer ID.
     */
    static void display(void* data, void* id);

    /**
     * @brief Cleans up the internal video context and VLC resources.
     */
    void cleanupContext();
};

#endif // VLC_PLAYER_H
