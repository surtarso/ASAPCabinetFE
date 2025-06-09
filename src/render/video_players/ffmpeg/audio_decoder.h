#ifndef AUDIO_DECODER_H
#define AUDIO_DECODER_H

#include <SDL2/SDL_audio.h>
#include <string>

// Forward declarations for FFmpeg structs
struct AVFormatContext;
struct AVCodecContext;
struct AVFrame;
struct AVPacket;
struct SwrContext;
struct AVAudioFifo;

class FFmpegPlayer; // Forward declaration

/**
 * @file audio_decoder.h
 * @brief Defines the AudioDecoder class for audio decoding in ASAPCabinetFE.
 *
 * This header provides the AudioDecoder class, which handles audio decoding using
 * FFmpeg and plays audio through SDL. It integrates with FFmpegPlayer for playback
 * control and supports volume adjustment and muting.
 */

/**
 * @class AudioDecoder
 * @brief Decodes and plays audio streams using FFmpeg and SDL.
 *
 * This class manages the decoding of audio streams from an AVFormatContext, resamples
 * audio to a compatible format, and buffers it for playback via SDL audio callbacks.
 * It works with FFmpegPlayer and handles volume and mute settings.
 */
class AudioDecoder {
public:
    /**
     * @brief Constructs an AudioDecoder instance.
     *
     * Initializes the decoder with a reference to the parent FFmpegPlayer.
     *
     * @param player Pointer to the FFmpegPlayer instance.
     */
    AudioDecoder(FFmpegPlayer* player);

    /**
     * @brief Destroys the AudioDecoder instance and cleans up resources.
     *
     * Ensures all allocated FFmpeg and SDL resources are properly freed.
     */
    ~AudioDecoder();

    /**
     * @brief Sets up the audio decoder with a format context.
     *
     * Configures the decoder with the audio stream and initializes SDL audio device.
     *
     * @param formatContext Pointer to the AVFormatContext containing the audio stream.
     * @return True on successful setup, false otherwise.
     */
    bool setup(AVFormatContext* formatContext);

    /**
     * @brief Starts audio playback.
     *
     * Unpauses the SDL audio device to begin playback.
     */
    void play();

    /**
     * @brief Stops audio playback.
     *
     * Pauses the SDL audio device and flushes the audio FIFO.
     */
    void stop();

    /**
     * @brief Updates the audio buffer by decoding frames.
     *
     * Ensures the audio FIFO has sufficient samples for continuous playback.
     */
    void update();

    /**
     * @brief Decodes the next audio frame.
     *
     * Attempts to read and decode an audio packet into a frame.
     *
     * @return True if a valid frame was decoded, false otherwise.
     */
    bool decodeAudioFrame();

    /**
     * @brief Fills the SDL audio stream buffer with decoded audio.
     *
     * Applies volume and mute settings to the audio data.
     *
     * @param stream Pointer to the audio buffer.
     * @param len Length of the audio buffer in bytes.
     */
    void fillAudioStream(Uint8* stream, int len);

    /**
     * @brief SDL audio callback function.
     *
     * Static callback invoked by SDL to fill the audio buffer.
     *
     * @param userdata Pointer to the AudioDecoder instance.
     * @param stream Pointer to the audio buffer.
     * @param len Length of the audio buffer in bytes.
     */
    static void SDLAudioCallback(void* userdata, Uint8* stream, int len);

    /**
     * @brief Sets the audio volume.
     *
     * Applies a logarithmic scale to the input volume (0-100) and updates the current volume.
     *
     * @param volume Volume level (0-100).
     */
    void setVolume(float volume);

    /**
     * @brief Toggles the mute state.
     *
     * @param mute True to mute, false to unmute.
     */
    void setMute(bool mute);

    /**
     * @brief Flushes the decoder buffers.
     *
     * Clears any pending frames in the codec context.
     */
    void flush();

private:
    friend class FFmpegPlayer; // Allow FFmpegPlayer to access private members
    FFmpegPlayer* player_;              ///< Pointer to the parent FFmpegPlayer instance.
    AVCodecContext* audioCodecContext_; ///< FFmpeg codec context for audio decoding.
    AVFrame* audioFrame_;               ///< FFmpeg frame for raw audio data.
    AVPacket* audioPacket_;             ///< FFmpeg packet for audio data.
    SwrContext* swrContext_;            ///< FFmpeg resampler context for audio conversion.
    AVAudioFifo* audioFifo_;            ///< FFmpeg audio FIFO for buffering decoded samples.
    int audioStreamIndex_;              ///< Index of the audio stream in the format context.
    SDL_AudioDeviceID audioDevice_;     ///< SDL audio device ID for playback.
    SDL_AudioSpec audioSpec_;           ///< SDL audio specification for the device.
    float currentVolume_;               ///< Current volume level (0.0 to 1.0).
    bool isMuted_;                      ///< Flag indicating mute state.

    /**
     * @brief Cleans up all allocated resources.
     *
     * Frees FFmpeg and SDL resources, resetting member pointers.
     */
    void cleanup();
};

#endif // AUDIO_DECODER_H