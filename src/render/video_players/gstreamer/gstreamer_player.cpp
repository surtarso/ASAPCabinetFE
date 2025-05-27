#include "gstreamer_player.h"
#include "utils/logging.h"
#include <gst/gst.h>
#include <gst/app/gstappsink.h>
#include <SDL2/SDL.h>
#include <string>
#include <sstream>
#include <glib.h> // Include GLib header for GMainLoop and GThread

// Static counter and mutex for GStreamer initialization
static int gstreamer_instance_count = 0;
static SDL_mutex* gstreamer_init_mutex = nullptr;

// Static GLib objects for the GStreamer event loop thread
static GMainLoop* g_main_loop = nullptr;
static GThread* g_main_loop_thread = nullptr;

// The function that the GStreamer event thread will run
// 'data' is unused, so we cast it to void to suppress the warning
static gpointer gstreamer_event_thread_func(gpointer data) {
    (void)data; // Cast to void to suppress "unused parameter" warning
    g_main_loop = g_main_loop_new(NULL, FALSE); // Create a new GMainLoop
    LOG_DEBUG("GStreamerVideoPlayer: GStreamer event thread started.");
    g_main_loop_run(g_main_loop); // Run the loop, blocking until g_main_loop_quit() is called
    LOG_DEBUG("GStreamerVideoPlayer: GStreamer event thread stopped.");
    return NULL;
}

GStreamerVideoPlayer::GStreamerVideoPlayer() : ctx_(nullptr) {
    if (!gstreamer_init_mutex) {
        gstreamer_init_mutex = SDL_CreateMutex();
        if (!gstreamer_init_mutex) {
            LOG_ERROR("GStreamerVideoPlayer: Failed to create init mutex");
        }
    }

    if (SDL_LockMutex(gstreamer_init_mutex) == 0) {
        if (gstreamer_instance_count == 0) {
            setenv("GST_DEBUG", "4", 1);
            setenv("GST_DEBUG_FILE", "logs/gstreamer.log", 1);
            gst_init(nullptr, nullptr);
            LOG_DEBUG("GStreamerVideoPlayer: Initialized GStreamer");

            if (!g_main_loop_thread) { // Only start if not already running
                // g_thread_supported() and g_thread_init() are deprecated and often unnecessary
                // for modern GLib versions (2.32+), as threading is initialized automatically.
                // Removed to silence deprecated warnings.
                g_main_loop_thread = g_thread_new("gstreamer-events", gstreamer_event_thread_func, NULL);
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
    if (SDL_LockMutex(gstreamer_init_mutex) == 0) {
        gstreamer_instance_count--;
        if (gstreamer_instance_count == 0) {
            if (g_main_loop) { // Only quit if loop is running
                g_main_loop_quit(g_main_loop);
                g_main_loop = nullptr; // Reset pointer after quitting
            }
            if (g_main_loop_thread) { // Wait for the thread to finish
                g_thread_join(g_main_loop_thread);
                g_thread_unref(g_main_loop_thread); // Unref the GThread object
                g_main_loop_thread = nullptr; // Reset pointer
                LOG_DEBUG("GStreamerVideoPlayer: GStreamer event thread joined and unref'd.");
            }
            gst_deinit();
            LOG_DEBUG("GStreamerVideoPlayer: Deinitialized GStreamer");
        }
        SDL_UnlockMutex(gstreamer_init_mutex);
    } else {
        LOG_ERROR("GStreamerVideoPlayer: Failed to lock init mutex for deinit");
    }
}

// 'bus' parameter is unused, cast to void to suppress the warning
static gboolean bus_call(GstBus* bus, GstMessage* msg, gpointer data) {
    (void)bus; // Cast to void to suppress "unused parameter" warning
    GstElement* pipeline = static_cast<GstElement*>(data);

    switch (GST_MESSAGE_TYPE(msg)) {
        case GST_MESSAGE_ERROR: {
            GError* err = nullptr;
            gchar* debug = nullptr;
            gst_message_parse_error(msg, &err, &debug);
            LOG_ERROR("GStreamerVideoPlayer: Error received from pipeline"); // Simplified log
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
                    gst_element_set_state(pipeline, GST_STATE_NULL); // Stop if seek fails
                }
            } else {
                LOG_ERROR("GStreamerVideoPlayer: Cannot seek, pipeline is null on EOS.");
            }
            break;
        }
        case GST_MESSAGE_STATE_CHANGED: {
            GstState old_state, new_state, pending_state;
            gst_message_parse_state_changed(msg, &old_state, &new_state, &pending_state);
            if (GST_MESSAGE_SRC(msg) == GST_OBJECT(pipeline)) {
                LOG_DEBUG("GStreamerVideoPlayer: Pipeline state changed."); // Simplified log
            }
            break;
        }
        case GST_MESSAGE_WARNING: {
            GError* err = nullptr;
            gchar* debug = nullptr;
            gst_message_parse_warning(msg, &err, &debug);
            LOG_DEBUG("GStreamerVideoPlayer: Warning received from pipeline."); // Simplified log
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
            }
            gst_bus_set_flushing(bus, TRUE);
            gst_object_unref(bus);
        }
        gst_element_set_state(ctx_->pipeline, GST_STATE_NULL);
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

    // Log SDL renderer pixel format
    SDL_RendererInfo info;
    if (SDL_GetRendererInfo(renderer, &info) == 0) {
        LOG_DEBUG("GStreamerVideoPlayer: SDL Renderer formats");
        for (Uint32 i = 0; i < info.num_texture_formats; ++i) {
            LOG_DEBUG("GStreamerVideoPlayer: Format found");
        }
    } else {
        LOG_ERROR("GStreamerVideoPlayer: Failed to get renderer info");
    }

    // Pipeline with RGBA, no audio handling.
    std::string pipeline_desc = "filesrc location=\"" + path + "\" ! decodebin ! videoconvert ! videorate ! videoscale ! video/x-raw,format=RGBA ! appsink name=videosink";
    GError* error = nullptr;
    ctx_->pipeline = gst_parse_launch(pipeline_desc.c_str(), &error);
    if (!ctx_->pipeline || error) {
        LOG_ERROR("GStreamerVideoPlayer: Failed to create pipeline");
        if (error) g_error_free(error);
        cleanupContext();
        return false;
    }

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

    // Get appsink
    GstElement* videosink = gst_bin_get_by_name(GST_BIN(ctx_->pipeline), "videosink");
    if (!videosink) {
        LOG_ERROR("GStreamerVideoPlayer: Failed to get appsink");
        cleanupContext();
        return false;
    }

    g_object_set(G_OBJECT(videosink), "emit-signals", TRUE, "sync", TRUE, nullptr);
    g_signal_connect(videosink, "new-sample", G_CALLBACK(onNewSample), this);

    // Set pipeline to PAUSED
    GstStateChangeReturn ret = gst_element_set_state(ctx_->pipeline, GST_STATE_PAUSED);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        LOG_ERROR("GStreamerVideoPlayer: Failed to set pipeline to PAUSED");
        gst_object_unref(videosink);
        cleanupContext();
        return false;
    }

    // Wait for state change (3 seconds)
    GstState state;
    ret = gst_element_get_state(ctx_->pipeline, &state, nullptr, 3 * GST_SECOND);
    if (ret == GST_STATE_CHANGE_FAILURE || state != GST_STATE_PAUSED) {
        LOG_ERROR("GStreamerVideoPlayer: Failed to reach PAUSED state");
        gst_object_unref(videosink);
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
            // 'format' variable is unused, cast to void to suppress the warning
            const char* format = gst_structure_get_string(structure, "format");
            (void)format; // Cast to void to suppress "unused variable" warning
            LOG_DEBUG("GStreamerVideoPlayer: Caps format found");
            if (gst_structure_get_int(structure, "width", &video_width) &&
                gst_structure_get_int(structure, "height", &video_height)) {
                LOG_DEBUG("GStreamerVideoPlayer: Video dimensions from caps");
                ctx_->width = video_width;
                ctx_->height = video_height;
            } else {
                LOG_DEBUG("GStreamerVideoPlayer: Using requested dimensions");
            }
            gst_caps_unref(pad_caps);
        } else {
            LOG_DEBUG("GStreamerVideoPlayer: No caps available");
        }
        gst_object_unref(pad);
    } else {
        LOG_ERROR("GStreamerVideoPlayer: Failed to get appsink pad");
        gst_object_unref(videosink);
        cleanupContext();
        return false;
    }
    gst_object_unref(videosink);

    // Create SDL texture with ABGR
    ctx_->texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ABGR8888,
                                      SDL_TEXTUREACCESS_STREAMING, ctx_->width, ctx_->height);
    if (!ctx_->texture) {
        LOG_ERROR("GStreamerVideoPlayer: Failed to create video texture");
        cleanupContext();
        return false;
    }

    // Allocate pixel buffer and frame buffer
    ctx_->pitch = ctx_->width * 4;
    ctx_->pixels = malloc(ctx_->pitch * ctx_->height);
    ctx_->frame_buffer.resize(ctx_->pitch * ctx_->height);
    if (!ctx_->pixels) {
        LOG_ERROR("GStreamerVideoPlayer: Failed to allocate pixel buffer");
        cleanupContext();
        return false;
    }

    LOG_DEBUG("GStreamerVideoPlayer: Setup complete for path");
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
    } else if (ctx_ && ctx_->isPlaying) {
        LOG_DEBUG("GStreamerVideoPlayer: Already playing");
    } else {
        LOG_ERROR("GStreamerVideoPlayer: Context or pipeline invalid for play");
    }
}

void GStreamerVideoPlayer::stop() {
    if (ctx_ && ctx_->pipeline) {
        gst_element_set_state(ctx_->pipeline, GST_STATE_NULL);
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
                LOG_ERROR("GStreamerVideoPlayer: SDL_UpdateTexture failed");
            }
            ctx_->frame_ready = false;
        }
        SDL_UnlockMutex(ctx_->mutex);
    } else {
        LOG_ERROR("GStreamerVideoPlayer: SDL_LockMutex failed");
    }
}

SDL_Texture* GStreamerVideoPlayer::getTexture() const {
    return ctx_ ? ctx_->texture : nullptr;
}

bool GStreamerVideoPlayer::isPlaying() const {
    return ctx_ ? ctx_->isPlaying : false;
}

// 'volume' is unused, cast to void to suppress the warning
void GStreamerVideoPlayer::setVolume(float volume) {
    (void)volume; // Cast to void to suppress "unused parameter" warning
    LOG_INFO("GStreamerVideoPlayer: Volume control not supported in this pipeline");
}

// 'mute' is unused, cast to void to suppress the warning
void GStreamerVideoPlayer::setMute(bool mute) {
    (void)mute; // Cast to void to suppress "unused parameter" warning
    LOG_INFO("GStreamerVideoPlayer: Mute control not supported in this pipeline");
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
            } else {
                LOG_ERROR("GStreamerVideoPlayer: SDL_LockMutex failed");
            }
        } else {
            LOG_ERROR("GStreamerVideoPlayer: Buffer size too small");
        }
        gst_buffer_unmap(buffer, &map);
    } else {
        LOG_ERROR("GStreamerVideoPlayer: Failed to map buffer");
    }

    gst_sample_unref(sample);
    return GST_FLOW_OK;
}