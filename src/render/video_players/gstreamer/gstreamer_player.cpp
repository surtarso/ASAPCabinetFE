#include "gstreamer_player.h"
#include "utils/logging.h"
#include <gst/gst.h>
#include <gst/app/gstappsink.h>
#include <SDL2/SDL.h>
#include <string>
#include <sstream>
#include <glib.h>

// Static counter and mutex for GStreamer initialization
static int gstreamer_instance_count = 0;
static SDL_mutex* gstreamer_init_mutex = nullptr;
static bool gstreamer_initialized = false; // Track if GStreamer was successfully initialized
static bool g_main_loop_running = false;  // Track if GMainLoop is running

// Static GLib objects for the GStreamer event loop thread
static GMainLoop* g_main_loop = nullptr;
static GThread* g_main_loop_thread = nullptr;

// Forward declaration of the event thread function
static gpointer gstreamer_event_thread_func(gpointer data);

GStreamerVideoPlayer::GStreamerVideoPlayer() : ctx_(nullptr) {
    if (!gstreamer_init_mutex) {
        gstreamer_init_mutex = SDL_CreateMutex();
        if (!gstreamer_init_mutex) {
            LOG_ERROR("GStreamerVideoPlayer: Failed to create init mutex");
            return;
        }
    }

    if (SDL_LockMutex(gstreamer_init_mutex) == 0) {
        if (gstreamer_instance_count == 0 && !gstreamer_initialized) {
            setenv("GST_DEBUG", "4", 1);
            setenv("GST_DEBUG_FILE", "logs/gstreamer.log", 1);
            gst_init(nullptr, nullptr);
            LOG_DEBUG("GStreamerVideoPlayer: GStreamer initialized with GST_DEBUG=4");
            gstreamer_initialized = true;
            LOG_DEBUG("GStreamerVideoPlayer: Initialized GStreamer");

            if (!g_main_loop_thread) {
                g_main_loop = g_main_loop_new(NULL, FALSE);
                if (!g_main_loop) {
                    LOG_ERROR("GStreamerVideoPlayer: Failed to create GMainLoop");
                    gst_deinit();
                    gstreamer_initialized = false;
                    SDL_UnlockMutex(gstreamer_init_mutex);
                    return;
                }
                LOG_DEBUG("GStreamerVideoPlayer: GStreamer event loop created.");
                g_main_loop_running = true;

                g_main_loop_thread = g_thread_new("gstreamer-events", gstreamer_event_thread_func, nullptr);
                if (!g_main_loop_thread) {
                    LOG_ERROR("GStreamerVideoPlayer: Failed to create GStreamer event thread");
                    g_main_loop_unref(g_main_loop);
                    g_main_loop = nullptr;
                    g_main_loop_running = false;
                    gst_deinit();
                    gstreamer_initialized = false;
                    SDL_UnlockMutex(gstreamer_init_mutex);
                    return;
                }
                LOG_DEBUG("GStreamerVideoPlayer: GStreamer event thread created.");
            }
        }
        gstreamer_instance_count++;
        SDL_UnlockMutex(gstreamer_init_mutex);
    } else {
        LOG_ERROR("GStreamerVideoPlayer: Failed to lock init mutex");
    }
}

GStreamerVideoPlayer::~GStreamerVideoPlayer() {
    cleanupContext();

    static bool deinit_in_progress = false; // Prevent recursive deinit
    if (SDL_LockMutex(gstreamer_init_mutex) == 0) {
        gstreamer_instance_count--;
        LOG_DEBUG("GStreamerVideoPlayer: Destructor, instance count: " + std::to_string(gstreamer_instance_count));

        if (gstreamer_instance_count == 0 && gstreamer_initialized && !deinit_in_progress) {
            deinit_in_progress = true; // Mark deinit started

            // Clean up GStreamer resources
            if (g_main_loop && g_main_loop_running) {
                g_main_loop_quit(g_main_loop);
                LOG_DEBUG("GStreamerVideoPlayer: GMainLoop quit requested in destructor.");
            }

            if (g_main_loop_thread) {
                g_thread_join(g_main_loop_thread);
                g_thread_unref(g_main_loop_thread);
                g_main_loop_thread = nullptr;
                LOG_DEBUG("GStreamerVideoPlayer: GStreamer event thread joined and unref’d.");
            }

            if (g_main_loop) {
                if (g_main_loop_running) {
                    g_main_loop_unref(g_main_loop);
                    LOG_DEBUG("GStreamerVideoPlayer: GMainLoop unref’d in destructor.");
                }
                g_main_loop = nullptr;
            }
            g_main_loop_running = false;

            if (gstreamer_initialized) {
                gst_deinit();
                gstreamer_initialized = false;
                LOG_DEBUG("GStreamerVideoPlayer: Deinitialized GStreamer");
            }

            deinit_in_progress = false; // Reset after completion
        }

        SDL_UnlockMutex(gstreamer_init_mutex);

        if (gstreamer_instance_count == 0 && gstreamer_init_mutex) {
            SDL_DestroyMutex(gstreamer_init_mutex);
            gstreamer_init_mutex = nullptr;
            LOG_DEBUG("GStreamerVideoPlayer: Destroyed init mutex");
        }
    } else {
        LOG_ERROR("GStreamerVideoPlayer: Failed to lock init mutex for deinit");
    }
}

static gpointer gstreamer_event_thread_func(gpointer data) {
    (void)data;
    g_main_loop = g_main_loop_new(NULL, FALSE);
    if (!g_main_loop) {
        LOG_ERROR("GStreamerVideoPlayer: Failed to create GMainLoop");
        g_main_loop_running = false;
        return NULL;
    }
    LOG_DEBUG("GStreamerVideoPlayer: GStreamer event loop created.");
    g_main_loop_running = true;
    g_main_loop_run(g_main_loop);
    LOG_DEBUG("GStreamerVideoPlayer: GStreamer event thread stopped.");
    g_main_loop_unref(g_main_loop);
    g_main_loop = nullptr;
    g_main_loop_running = false;
    LOG_DEBUG("GStreamerVideoPlayer: GMainLoop unref'd in thread.");
    return NULL;
}

static gboolean bus_call(GstBus* bus, GstMessage* msg, gpointer data) {
    (void)bus;
    GstElement* pipeline = static_cast<GstElement*>(data);

    switch (GST_MESSAGE_TYPE(msg)) {
        case GST_MESSAGE_ERROR: {
            GError* err = nullptr;
            gchar* debug = nullptr;
            gst_message_parse_error(msg, &err, &debug);
            LOG_ERROR("GStreamerVideoPlayer: Error received from pipeline: " + std::string(err ? err->message : "unknown"));
            if (err) g_error_free(err);
            if (debug) g_free(debug);
            if (pipeline) {
                gst_element_set_state(pipeline, GST_STATE_NULL);
            }
            break;
        }
        case GST_MESSAGE_EOS: {
            LOG_DEBUG("GStreamerVideoPlayer: EOS reached. Attempting to seek to start.");
            if (pipeline) {
                gboolean seek_success = gst_element_seek(
                    pipeline,
                    1.0,
                    GST_FORMAT_TIME,
                    static_cast<GstSeekFlags>(GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_KEY_UNIT),
                    GST_SEEK_TYPE_SET,
                    0,
                    GST_SEEK_TYPE_NONE,
                    -1
                );
                if (seek_success) {
                    LOG_DEBUG("GStreamerVideoPlayer: Seek to start succeeded. Resuming playback.");
                    gst_element_set_state(pipeline, GST_STATE_PLAYING);
                } else {
                    LOG_ERROR("GStreamerVideoPlayer: Seek to start FAILED!");
                    gst_element_set_state(pipeline, GST_STATE_NULL);
                }
            }
            break;
        }
        case GST_MESSAGE_STATE_CHANGED: {
            GstState old_state, new_state, pending_state;
            gst_message_parse_state_changed(msg, &old_state, &new_state, &pending_state);
            break;
        }
        case GST_MESSAGE_WARNING: {
            GError* err = nullptr;
            gchar* debug = nullptr;
            gst_message_parse_warning(msg, &err, &debug);
            LOG_DEBUG("GStreamerVideoPlayer: Warning received from pipeline: " + std::string(err ? err->message : "unknown"));
            if (err) g_error_free(err);
            if (debug) g_free(debug);
            break;
        }
        default:
            break;
    }
    return TRUE;
}

void GStreamerVideoPlayer::cleanupContext() {
    if (!ctx_) return;

    if (ctx_->pipeline) {
        GstBus* bus = gst_element_get_bus(ctx_->pipeline);
        if (bus) {
            if (ctx_->bus_watch_id) {
                g_source_remove(ctx_->bus_watch_id);
                ctx_->bus_watch_id = 0;
                LOG_DEBUG("GStreamerVideoPlayer: Bus watch removed.");
            }
            gst_bus_set_flushing(bus, TRUE);
            gst_object_unref(bus);
        }
        GstStateChangeReturn ret = gst_element_set_state(ctx_->pipeline, GST_STATE_NULL);
        if (ret == GST_STATE_CHANGE_FAILURE) {
            LOG_ERROR("GStreamerVideoPlayer: Failed to set pipeline to NULL during cleanup");
        }
        gst_object_unref(ctx_->pipeline);
        ctx_->pipeline = nullptr;
    }

    if (ctx_->texture) {
        SDL_DestroyTexture(ctx_->texture);
        ctx_->texture = nullptr;
    }
    if (ctx_->pixels) {
        free(ctx_->pixels);
        ctx_->pixels = nullptr;
    }
    if (ctx_->mutex) {
        SDL_DestroyMutex(ctx_->mutex);
        ctx_->mutex = nullptr;
    }

    delete ctx_;
    ctx_ = nullptr;
}

bool GStreamerVideoPlayer::setup(SDL_Renderer* renderer, const std::string& path, int width, int height) {
    ctx_ = new VideoContext();
    ctx_->renderer = renderer;
    ctx_->width = width;
    ctx_->height = height;
    ctx_->isPlaying = false;
    ctx_->frame_ready = false;
    ctx_->first_frame = true;
    ctx_->mutex = SDL_CreateMutex();
    ctx_->current_path = path;
    ctx_->bus_watch_id = 0;
    if (!ctx_->mutex) {
        LOG_ERROR("GStreamerVideoPlayer: Failed to create mutex");
        cleanupContext();
        return false;
    }

    // Debug GStreamer version and plugin availability
    LOG_DEBUG("GStreamerVideoPlayer: GStreamer version: " + std::to_string(GST_VERSION_MAJOR) + "." +
              std::to_string(GST_VERSION_MINOR) + "." + std::to_string(GST_VERSION_MICRO));

    // Create pipeline with detailed error handling
    GError* error = nullptr;
    ctx_->pipeline = gst_pipeline_new("video-pipeline");
    if (!ctx_->pipeline) {
        std::string error_msg = error && error->message ? error->message : "Unknown error";
        LOG_ERROR("GStreamerVideoPlayer: Failed to create pipeline: " + error_msg);
        if (error) g_error_free(error);
        cleanupContext();
        return false;
    }

    // Create and configure filesrc
    GstElement* filesrc = gst_element_factory_make("filesrc", "filesrc");
    GstElement* decodebin = gst_element_factory_make("decodebin", "decodebin");
    if (!filesrc || !decodebin) {
        LOG_ERROR("GStreamerVideoPlayer: Failed to create pipeline elements: filesrc=" + std::to_string(filesrc != nullptr) + ", decodebin=" + std::to_string(decodebin != nullptr));
        LOG_ERROR("GStreamerVideoPlayer: Check if gstreamer1.0-plugins-base is installed");
        if (filesrc) gst_object_unref(filesrc);
        if (decodebin) gst_object_unref(decodebin);
        cleanupContext();
        return false;
    }
    g_object_set(G_OBJECT(filesrc), "location", path.c_str(), nullptr);

    // Add elements to pipeline
    gst_bin_add_many(GST_BIN(ctx_->pipeline), filesrc, decodebin, nullptr);

    // Link filesrc to decodebin
    if (!gst_element_link(filesrc, decodebin)) {
        LOG_ERROR("GStreamerVideoPlayer: Failed to link filesrc to decodebin");
        gst_object_unref(filesrc);
        gst_object_unref(decodebin);
        cleanupContext();
        return false;
    }

    // Connect pad-added signal
    g_signal_connect(decodebin, "pad-added", G_CALLBACK(onPadAdded), this);

    // Add bus watch
    GstBus* bus = gst_pipeline_get_bus(GST_PIPELINE(ctx_->pipeline));
    if (bus) {
        ctx_->bus_watch_id = gst_bus_add_watch(bus, bus_call, ctx_->pipeline);
        if (ctx_->bus_watch_id == 0) {
            LOG_ERROR("GStreamerVideoPlayer: Failed to add bus watch");
        } else {
            LOG_DEBUG("GStreamerVideoPlayer: Bus watch added successfully");
        }
        gst_object_unref(bus);
    }

    // Set pipeline to PAUSED
    GstStateChangeReturn ret = gst_element_set_state(ctx_->pipeline, GST_STATE_PAUSED);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        LOG_ERROR("GStreamerVideoPlayer: Failed to set pipeline to PAUSED");
        cleanupContext();
        return false;
    }

    // Wait for state change
    GstState state;
    ret = gst_element_get_state(ctx_->pipeline, &state, nullptr, 3 * GST_SECOND);
    if (ret == GST_STATE_CHANGE_FAILURE || state != GST_STATE_PAUSED) {
        LOG_ERROR("GStreamerVideoPlayer: Failed to reach PAUSED state");
        cleanupContext();
        return false;
    }

    // Get appsink
    GstElement* videosink = gst_bin_get_by_name(GST_BIN(ctx_->pipeline), "videosink");
    if (!videosink) {
        LOG_ERROR("GStreamerVideoPlayer: Failed to get appsink");
        cleanupContext();
        return false;
    }

    // Query video dimensions
    int video_width = width;
    int video_height = height;
    GstPad* pad = gst_element_get_static_pad(videosink, "sink");
    if (pad) {
        GstCaps* pad_caps = gst_pad_get_current_caps(pad);
        if (pad_caps) {
            GstStructure* structure = gst_caps_get_structure(pad_caps, 0);
            (void)gst_structure_get_string(structure, "format");
            LOG_DEBUG("GStreamerVideoPlayer: Caps format found");
            if (gst_structure_get_int(structure, "width", &video_width) &&
                gst_structure_get_int(structure, "height", &video_height)) {
                LOG_DEBUG("GStreamerVideoPlayer: Video dimensions from caps: width=" + std::to_string(video_width) + ", height=" + std::to_string(video_height));
                ctx_->width = video_width;
                ctx_->height = video_height;
            }
            gst_caps_unref(pad_caps);
        }
        gst_object_unref(pad);
    } else {
        LOG_ERROR("GStreamerVideoPlayer: Failed to get appsink pad");
        gst_object_unref(videosink);
        cleanupContext();
        return false;
    }
    gst_object_unref(videosink);

    // Create SDL texture
    ctx_->texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ABGR8888,
                                      SDL_TEXTUREACCESS_STREAMING, ctx_->width, ctx_->height);
    if (!ctx_->texture) {
        LOG_ERROR("GStreamerVideoPlayer: Failed to create video texture: " + std::string(SDL_GetError()));
        cleanupContext();
        return false;
    }

    // Allocate buffers
    ctx_->pitch = ctx_->width * 4;
    ctx_->pixels = malloc(ctx_->pitch * ctx_->height);
    ctx_->frame_buffer.resize(ctx_->pitch * ctx_->height);
    if (!ctx_->pixels) {
        LOG_ERROR("GStreamerVideoPlayer: Failed to allocate pixel buffer");
        cleanupContext();
        return false;
    }

    LOG_DEBUG("GStreamerVideoPlayer: Setup complete for path: " + path);
    return true;
}

void GStreamerVideoPlayer::play() {
    if (ctx_ && ctx_->pipeline && !ctx_->isPlaying) {
        GstStateChangeReturn ret = gst_element_set_state(ctx_->pipeline, GST_STATE_PLAYING);
        if (ret == GST_STATE_CHANGE_FAILURE) {
            LOG_ERROR("GStreamerVideoPlayer: Failed to start playback");
        } else {
            ctx_->isPlaying = true;
            LOG_DEBUG("GStreamerVideoPlayer: Playback started");
        }
    }
}

void GStreamerVideoPlayer::stop() {
    if (ctx_ && ctx_->pipeline) {
        GstStateChangeReturn ret = gst_element_set_state(ctx_->pipeline, GST_STATE_NULL);
        if (ret == GST_STATE_CHANGE_FAILURE) {
            LOG_ERROR("GStreamerVideoPlayer: Failed to set pipeline to NULL in stop");
        }
        ctx_->isPlaying = false;
        ctx_->frame_ready = false;
        ctx_->first_frame = true;
        LOG_DEBUG("GStreamerVideoPlayer: Playback stopped");
    }
}

void GStreamerVideoPlayer::update() {
    if (!ctx_ || !ctx_->texture || !ctx_->pixels || !ctx_->mutex || !ctx_->isPlaying) {
        return;
    }

    if (SDL_LockMutex(ctx_->mutex) == 0) {
        if (ctx_->frame_ready) {
            memcpy(ctx_->pixels, ctx_->frame_buffer.data(), ctx_->pitch * ctx_->height);
            if (SDL_UpdateTexture(ctx_->texture, nullptr, ctx_->pixels, ctx_->pitch) != 0) {
                LOG_ERROR("GStreamerVideoPlayer: SDL_UpdateTexture failed: " + std::string(SDL_GetError()));
            }
            ctx_->frame_ready = false;
        }
        SDL_UnlockMutex(ctx_->mutex);
    }
}

SDL_Texture* GStreamerVideoPlayer::getTexture() const {
    return ctx_ ? ctx_->texture : nullptr;
}

bool GStreamerVideoPlayer::isPlaying() const {
    return ctx_ ? ctx_->isPlaying : false;
}

void GStreamerVideoPlayer::setVolume(float volume) {
    if (ctx_ && ctx_->volume_element) {
        double scaled_volume = static_cast<double>(volume) / 100.0;
        g_object_set(G_OBJECT(ctx_->volume_element), "volume", scaled_volume, nullptr);
        LOG_DEBUG("GStreamerVideoPlayer: Set volume to " + std::to_string(scaled_volume));
    } else {
        LOG_INFO("GStreamerVideoPlayer: No audio track, cannot set volume");
    }
}

void GStreamerVideoPlayer::setMute(bool mute) {
    if (ctx_ && ctx_->volume_element) {
        g_object_set(G_OBJECT(ctx_->volume_element), "mute", static_cast<gboolean>(mute), nullptr);
        LOG_DEBUG("GStreamerVideoPlayer: Set mute to " + std::string(mute ? "true" : "false"));
    } else {
        LOG_INFO("GStreamerVideoPlayer: No audio track, cannot set mute");
    }
}

std::string GStreamerVideoPlayer::getCurrentPath() const {
    return ctx_ ? ctx_->current_path : "";
}

GstFlowReturn GStreamerVideoPlayer::onNewSample(GstAppSink* appsink, void* data) {
    GStreamerVideoPlayer* player = static_cast<GStreamerVideoPlayer*>(data);
    GstSample* sample = gst_app_sink_pull_sample(appsink);
    if (!sample) {
        LOG_ERROR("GStreamerVideoPlayer: Failed to pull sample from appsink");
        return GST_FLOW_OK;
    }

    GstBuffer* buffer = gst_sample_get_buffer(sample);
    if (!buffer) {
        gst_sample_unref(sample);
        LOG_ERROR("GStreamerVideoPlayer: Sample has no buffer");
        return GST_FLOW_OK;
    }

    GstMapInfo map;
    if (gst_buffer_map(buffer, &map, GST_MAP_READ)) {
        size_t expected_size = player->ctx_->pitch * player->ctx_->height;
        if (map.size >= expected_size) {
            if (player->ctx_->first_frame) {
                LOG_DEBUG("GStreamerVideoPlayer: First frame received");
                player->ctx_->first_frame = false;
            }
            if (SDL_LockMutex(player->ctx_->mutex) == 0) {
                memcpy(player->ctx_->frame_buffer.data(), map.data, expected_size);
                player->ctx_->frame_ready = true;
                SDL_UnlockMutex(player->ctx_->mutex);
            }
        }
        gst_buffer_unmap(buffer, &map);
    }
    gst_sample_unref(sample);
    return GST_FLOW_OK;
}

void GStreamerVideoPlayer::onPadAdded(GstElement* decodebin, GstPad* pad, gpointer data) {
    (void)decodebin;
    GStreamerVideoPlayer* player = static_cast<GStreamerVideoPlayer*>(data);
    GstCaps* caps = gst_pad_get_current_caps(pad);
    if (!caps) {
        caps = gst_pad_query_caps(pad, nullptr);
    }
    if (caps) {
        std::string caps_str = gst_caps_to_string(caps);
        if (g_str_has_prefix(caps_str.c_str(), "video/")) {
            player->linkVideoPad(pad);
        } else if (g_str_has_prefix(caps_str.c_str(), "audio/")) {
            player->linkAudioPad(pad);
        }
        gst_caps_unref(caps);
    }
}

void GStreamerVideoPlayer::linkVideoPad(GstPad* pad) {
    GstElement* videoconvert = gst_element_factory_make("videoconvert", "videoconvert");
    GstElement* videorate = gst_element_factory_make("videorate", "videorate");
    GstElement* videoscale = gst_element_factory_make("videoscale", "videoscale");
    GstElement* capsfilter = gst_element_factory_make("capsfilter", "capsfilter");
    GstElement* appsink = gst_element_factory_make("appsink", "videosink");

    if (!videoconvert || !videorate || !videoscale || !capsfilter || !appsink) {
        LOG_ERROR("GStreamerVideoPlayer: Failed to create video elements: videoconvert=" + std::to_string(videoconvert != nullptr) +
                  ", videorate=" + std::to_string(videorate != nullptr) +
                  ", videoscale=" + std::to_string(videoscale != nullptr) +
                  ", capsfilter=" + std::to_string(capsfilter != nullptr) +
                  ", appsink=" + std::to_string(appsink != nullptr));
        LOG_ERROR("GStreamerVideoPlayer: Check if gstreamer1.0-plugins-base and gstreamer1.0-plugins-good are installed");
        if (videoconvert) gst_object_unref(videoconvert);
        if (videorate) gst_object_unref(videorate);
        if (videoscale) gst_object_unref(videoscale);
        if (capsfilter) gst_object_unref(capsfilter);
        if (appsink) gst_object_unref(appsink);
        return;
    }

    GstCaps* caps = gst_caps_from_string("video/x-raw,format=RGBA");
    g_object_set(G_OBJECT(capsfilter), "caps", caps, nullptr);
    gst_caps_unref(caps);

    gst_bin_add_many(GST_BIN(ctx_->pipeline), videoconvert, videorate, videoscale, capsfilter, appsink, nullptr);

    if (!gst_element_link_many(videoconvert, videorate, videoscale, capsfilter, appsink, nullptr)) {
        LOG_ERROR("GStreamerVideoPlayer: Failed to link video elements");
        gst_element_set_state(ctx_->pipeline, GST_STATE_NULL);
        return;
    }

    GstPad* sink_pad = gst_element_get_static_pad(videoconvert, "sink");
    if (sink_pad) {
        if (gst_pad_link(pad, sink_pad) != GST_PAD_LINK_OK) {
            LOG_ERROR("GStreamerVideoPlayer: Failed to link decodebin pad to videoconvert");
        }
        gst_object_unref(sink_pad);
    }

    gst_element_sync_state_with_parent(videoconvert);
    gst_element_sync_state_with_parent(videorate);
    gst_element_sync_state_with_parent(videoscale);
    gst_element_sync_state_with_parent(capsfilter);
    gst_element_sync_state_with_parent(appsink);

    g_object_set(G_OBJECT(appsink), "emit-signals", TRUE, "sync", TRUE, nullptr);
    g_signal_connect(appsink, "new-sample", G_CALLBACK(onNewSample), this);
}

void GStreamerVideoPlayer::linkAudioPad(GstPad* pad) {
    GstElement* audioconvert = gst_element_factory_make("audioconvert", "audioconvert");
    GstElement* volume = gst_element_factory_make("volume", "volume");
    GstElement* audiosink = gst_element_factory_make("autoaudiosink", "audiosink");

    if (!audioconvert || !volume || !audiosink) {
        LOG_ERROR("GStreamerVideoPlayer: Failed to create audio elements: audioconvert=" + std::to_string(audioconvert != nullptr) +
                  ", volume=" + std::to_string(volume != nullptr) +
                  ", audiosink=" + std::to_string(audiosink != nullptr));
        LOG_ERROR("GStreamerVideoPlayer: Check if gstreamer1.0-plugins-base and gstreamer1.0-plugins-good are installed");
        if (audioconvert) gst_object_unref(audioconvert);
        if (volume) gst_object_unref(volume);
        if (audiosink) gst_object_unref(audiosink);
        return;
    }

    gst_bin_add_many(GST_BIN(ctx_->pipeline), audioconvert, volume, audiosink, nullptr);

    if (!gst_element_link_many(audioconvert, volume, audiosink, nullptr)) {
        LOG_ERROR("GStreamerVideoPlayer: Failed to link audio elements");
        gst_element_set_state(ctx_->pipeline, GST_STATE_NULL);
        return;
    }

    GstPad* sink_pad = gst_element_get_static_pad(audioconvert, "sink");
    if (sink_pad) {
        if (gst_pad_link(pad, sink_pad) != GST_PAD_LINK_OK) {
            LOG_ERROR("GStreamerVideoPlayer: Failed to link decodebin pad to audioconvert");
        }
        gst_object_unref(sink_pad);
    }

    gst_element_sync_state_with_parent(audioconvert);
    gst_element_sync_state_with_parent(volume);
    gst_element_sync_state_with_parent(audiosink);

    ctx_->volume_element = volume;
}