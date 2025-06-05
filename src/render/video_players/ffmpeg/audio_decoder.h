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

class AudioDecoder {
public:
    AudioDecoder(FFmpegPlayer* player);
    ~AudioDecoder();

    bool setup(AVFormatContext* formatContext);
    void play();
    void stop();
    void update();
    bool decodeAudioFrame();
    void fillAudioStream(Uint8* stream, int len);
    static void SDLAudioCallback(void* userdata, Uint8* stream, int len);
    void setVolume(float volume);
    void setMute(bool mute);
    void flush();

private:
    friend class FFmpegPlayer; // Allow FFmpegPlayer to access private members
    FFmpegPlayer* player_;
    AVCodecContext* audioCodecContext_;
    AVFrame* audioFrame_;
    AVPacket* audioPacket_;
    SwrContext* swrContext_;
    AVAudioFifo* audioFifo_;
    int audioStreamIndex_;
    SDL_AudioDeviceID audioDevice_;
    SDL_AudioSpec audioSpec_;
    float currentVolume_;
    bool isMuted_;

    void cleanup();
};

#endif // AUDIO_DECODER_H