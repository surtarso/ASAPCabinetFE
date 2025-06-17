#include "gstreamer_player.h"
#include "log/logging.h"
#include <gst/gst.h>
#include <gst/app/gstappsink.h>
#include <SDL2/SDL.h>
#include <string>
#include <sstream>
#include <glib.h> // Ensure glib is included for Glib functions

// Static counter and mutex for GStreamer instance management
static int gstreamer_instance_count = 0;
static SDL_mutex* gstreamer_init_mutex = nullptr;
static bool gstreamer_initialized = false; // This now indicates if gst_init has been called globally (in main.cpp)

// REMOVED: Static GLib objects for the GStreamer event loop thread
// These are now managed globally in main.cpp.
// static GMainLoop* g_main_loop = nullptr;
// static GThread* g_main_loop_thread = nullptr;
// static bool g_main_loop_running = false;

// REMOVED: Forward declaration AND definition of the event thread function
// static gpointer gstreamer_event_thread_func(gpointer data);


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
            LOG_DEBUG("GStreamerVideoPlayer: GStreamer debug level set.");
            // We assume gst_init() has been called globally in main.cpp
            gstreamer_initialized = true;
            LOG_DEBUG("GStreamerVideoPlayer: Global GStreamer environment marked as ready for GStreamerVideoPlayer instances.");
        }
        gstreamer_instance_count++;
        SDL_UnlockMutex(gstreamer_init_mutex);
    } else {
        LOG_ERROR("GStreamerVideoPlayer: Failed to lock init mutex");
    }
}

GStreamerVideoPlayer::~GStreamerVideoPlayer() {
    cleanupContext(); // Call cleanupContext first to tear down pipeline resources

    if (SDL_LockMutex(gstreamer_init_mutex) == 0) {
        gstreamer_instance_count--;
        LOG_DEBUG("GStreamerVideoPlayer: Destructor, instance count: " + std::to_string(gstreamer_instance_count));

        // Only destroy the mutex if no more instances are active
        if (gstreamer_instance_count == 0) {
            if (gstreamer_init_mutex) {
                SDL_DestroyMutex(gstreamer_init_mutex);
                gstreamer_init_mutex = nullptr;
                LOG_DEBUG("GStreamerVideoPlayer: Destroyed init mutex.");
            }
            gstreamer_initialized = false; // Reset for next potential run of the entire app
        }
        SDL_UnlockMutex(gstreamer_init_mutex);
    } else {
        LOG_ERROR("GStreamerVideoPlayer: Failed to lock init mutex for deinit in destructor.");
    }
}

// bus_call (no changes needed)
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
            //LOG_DEBUG("GStreamerVideoPlayer: EOS reached. Attempting to seek to start.");
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
                    //LOG_DEBUG("GStreamerVideoPlayer: Seek to start succeeded. Resuming playback.");
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
        // Get decodebin, set to NULL, and unref it first
        GstElement* decodebin_temp = gst_bin_get_by_name(GST_BIN(ctx_->pipeline), "decodebin");
        if (decodebin_temp) {
            LOG_DEBUG("GStreamerVideoPlayer: Setting decodebin to NULL for cleanup.");
            gst_element_set_state(decodebin_temp, GST_STATE_NULL);
            gst_element_get_state(decodebin_temp, nullptr, nullptr, GST_CLOCK_TIME_NONE); // Wait for state change
            g_signal_handlers_disconnect_by_func(decodebin_temp, (gpointer)onPadAdded, this); // Disconnect signal
            gst_object_unref(decodebin_temp); // Release the temporary reference
            LOG_DEBUG("GStreamerVideoPlayer: Decodebin unref'd.");
        }

        // Disconnect signals from stored appsink
        if (ctx_->videosink_element) {
            g_signal_handlers_disconnect_by_func(ctx_->videosink_element, (gpointer)onNewSample, this);
            LOG_DEBUG("GStreamerVideoPlayer: Disconnected new-sample signal from appsink (using stored ref).");
        }

        // Set stored elements to NULL without unreferencing (except videosink if needed)
        if (ctx_->videosink_element) {
            LOG_DEBUG("GStreamerVideoPlayer: Setting stored videosink_element to NULL.");
            gst_element_set_state(ctx_->videosink_element, GST_STATE_NULL);
            gst_element_get_state(ctx_->videosink_element, nullptr, nullptr, GST_CLOCK_TIME_NONE);
            gst_object_unref(ctx_->videosink_element); // Unref only videosink due to signal connection
            ctx_->videosink_element = nullptr;
            LOG_DEBUG("GStreamerVideoPlayer: Stored videosink_element unreferenced.");
        }
        if (ctx_->audiosink_element) {
            LOG_DEBUG("GStreamerVideoPlayer: Setting stored audiosink_element to NULL.");
            gst_element_set_state(ctx_->audiosink_element, GST_STATE_NULL);
            gst_element_get_state(ctx_->audiosink_element, nullptr, nullptr, GST_CLOCK_TIME_NONE);
            ctx_->audiosink_element = nullptr; // Do not unref, let pipeline handle it
            LOG_DEBUG("GStreamerVideoPlayer: Stored audiosink_element set to nullptr.");
        }
        if (ctx_->volume_element) {
            LOG_DEBUG("GStreamerVideoPlayer: Setting stored volume_element to NULL.");
            gst_element_set_state(ctx_->volume_element, GST_STATE_NULL);
            gst_element_get_state(ctx_->volume_element, nullptr, nullptr, GST_CLOCK_TIME_NONE);
            ctx_->volume_element = nullptr; // Do not unref, let pipeline handle it
            LOG_DEBUG("GStreamerVideoPlayer: Stored volume_element set to nullptr.");
        }

        // Set the main pipeline to NULL to propagate state changes to children
        LOG_DEBUG("GStreamerVideoPlayer: Setting main pipeline to NULL for cleanup.");
        GstStateChangeReturn ret = gst_element_set_state(ctx_->pipeline, GST_STATE_NULL);
        if (ret == GST_STATE_CHANGE_FAILURE) {
            LOG_ERROR("GStreamerVideoPlayer: Failed to set pipeline to NULL during cleanup");
        }
        gst_element_get_state(ctx_->pipeline, nullptr, nullptr, GST_CLOCK_TIME_NONE);
        LOG_DEBUG("GStreamerVideoPlayer: Pipeline reached NULL state.");

        // Remove bus watch
        GstBus* bus = gst_element_get_bus(ctx_->pipeline);
        if (bus) {
            if (ctx_->bus_watch_id) {
                g_source_remove(ctx_->bus_watch_id);
                ctx_->bus_watch_id = 0;
                LOG_DEBUG("GStreamerVideoPlayer: Bus watch removed.");
            }
            gst_object_unref(bus);
        }

        // Unref the pipeline, which will clean up its children
        gst_object_unref(ctx_->pipeline);
        ctx_->pipeline = nullptr;
        LOG_DEBUG("GStreamerVideoPlayer: Pipeline unreferenced.");
    }

    // SDL and custom memory cleanup
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
    LOG_DEBUG("GStreamerVideoPlayer: VideoContext cleaned up.");
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
    ctx_->volume_element = nullptr; // Initialize volume element to nullptr
    ctx_->audiosink_element = nullptr; // Initialize new member
    ctx_->videosink_element = nullptr; // Initialize new member

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
        std::string error_msg = error && error->message ? error->message : "Unknown error (gst_pipeline_new)";
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
        LOG_ERROR("GStreamerVideoPlayer: Check if gstreamer1.0-plugins-base is installed.");
        if (filesrc) gst_object_unref(filesrc);
        if (decodebin) gst_object_unref(decodebin);
        cleanupContext();
        return false;
    }
    g_object_set(G_OBJECT(filesrc), "location", path.c_str(), nullptr);

    // Add elements to pipeline
    gst_bin_add_many(GST_BIN(ctx_->pipeline), filesrc, decodebin, nullptr);
    
    // Debug dot file dump - only if GST_DEBUG_DUMP_DOT_DIR env var is set
    // GST_DEBUG_BIN_TO_DOT_FILE_WITH_TS(GST_BIN(ctx_->pipeline), GST_DEBUG_GRAPH_SHOW_ALL, "pipeline-initial");

    // Link filesrc to decodebin
    if (!gst_element_link(filesrc, decodebin)) {
        LOG_ERROR("GStreamerVideoPlayer: Failed to link filesrc to decodebin.");
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
            LOG_ERROR("GStreamerVideoPlayer: Failed to add bus watch.");
        } else {
            LOG_DEBUG("GStreamerVideoPlayer: Bus watch added successfully.");
        }
        gst_object_unref(bus);
    }

    // Set pipeline to PAUSED
    GstStateChangeReturn ret = gst_element_set_state(ctx_->pipeline, GST_STATE_PAUSED);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        LOG_ERROR("GStreamerVideoPlayer: Failed to set pipeline to PAUSED.");
        cleanupContext();
        return false;
    }

    // Wait for state change to ensure it's truly paused
    GstState state;
    ret = gst_element_get_state(ctx_->pipeline, &state, nullptr, 3 * GST_SECOND);
    if (ret == GST_STATE_CHANGE_FAILURE || state != GST_STATE_PAUSED) {
        LOG_ERROR("GStreamerVideoPlayer: Failed to reach PAUSED state (returned " + std::to_string(ret) + ", current state " + std::string(gst_element_state_get_name(state)) + ").");
        cleanupContext();
        return false;
    }

    GstElement* videosink = gst_bin_get_by_name(GST_BIN(ctx_->pipeline), "videosink");
    if (!videosink) {
        LOG_DEBUG("GStreamerVideoPlayer: Could not get appsink 'videosink' at setup. This is expected if 'onPadAdded' hasn't run yet for video, but might indicate an issue if video is expected.");
    } else {
        int video_width = width;
        int video_height = height;
        GstPad* pad = gst_element_get_static_pad(videosink, "sink");
        if (pad) {
            GstCaps* pad_caps = gst_pad_get_current_caps(pad);
            if (pad_caps) {
                GstStructure* structure = gst_caps_get_structure(pad_caps, 0);
                (void)gst_structure_get_string(structure, "format");
                LOG_DEBUG("GStreamerVideoPlayer: Caps format found.");
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
            LOG_ERROR("GStreamerVideoPlayer: Failed to get appsink pad (even if sink exists).");
        }
        gst_object_unref(videosink);
    }


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
        LOG_ERROR("GStreamerVideoPlayer: Failed to allocate pixel buffer.");
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
    if (!player || !player->ctx_ || !player->ctx_->mutex || !player->ctx_->pixels) {
        LOG_ERROR("GStreamerVideoPlayer: onNewSample called with invalid player context.");
        return GST_FLOW_ERROR;
    }
    GstSample* sample = gst_app_sink_pull_sample(appsink);
    if (!sample) {
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
        if (map.size >= expected_size && map.data) {
            if (player->ctx_->first_frame) {
                LOG_DEBUG("GStreamerVideoPlayer: First frame received");
                player->ctx_->first_frame = false;
            }
            if (SDL_LockMutex(player->ctx_->mutex) == 0) {
                memcpy(player->ctx_->frame_buffer.data(), map.data, expected_size);
                player->ctx_->frame_ready = true;
                SDL_UnlockMutex(player->ctx_->mutex);
            }
        } else {
             LOG_DEBUG("GStreamerVideoPlayer: Mapped buffer size (" + std::to_string(map.size) + ") less than expected (" + std::to_string(expected_size) + ") or data is null.");
        }
        gst_buffer_unmap(buffer, &map);
    } else {
        LOG_ERROR("GStreamerVideoPlayer: Failed to map buffer.");
    }
    gst_sample_unref(sample);
    return GST_FLOW_OK;
}

void GStreamerVideoPlayer::onPadAdded(GstElement* decodebin, GstPad* pad, gpointer data) {
    (void)decodebin;
    GStreamerVideoPlayer* player = static_cast<GStreamerVideoPlayer*>(data);
    if (!player) {
        LOG_ERROR("GStreamerVideoPlayer: onPadAdded called with null player data.");
        return;
    }
    GstCaps* caps = gst_pad_get_current_caps(pad);
    if (!caps) {
        caps = gst_pad_query_caps(pad, nullptr);
    }
    if (caps) {
        gchar* caps_raw = gst_caps_to_string(caps); // Get raw string
        if (caps_raw) {
            std::string caps_str = caps_raw; // Convert to std::string
            g_free(caps_raw); // Free the allocated string
            if (g_str_has_prefix(caps_str.c_str(), "video/")) {
                player->linkVideoPad(pad);
            } else if (g_str_has_prefix(caps_str.c_str(), "audio/")) {
                player->linkAudioPad(pad);
            }
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
        LOG_ERROR("GStreamerVideoPlayer: Failed to create video elements. Check if gstreamer1.0-plugins-base and gstreamer1.0-plugins-good are installed.");
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

    g_object_set(G_OBJECT(appsink), "emit-signals", TRUE, "sync", TRUE, nullptr);
    g_signal_connect(appsink, "new-sample", G_CALLBACK(onNewSample), this);
    ctx_->videosink_element = GST_ELEMENT(gst_object_ref(appsink)); // Cast added here

    gst_element_sync_state_with_parent(videoconvert);
    gst_element_sync_state_with_parent(videorate);
    gst_element_sync_state_with_parent(videoscale);
    gst_element_sync_state_with_parent(capsfilter);
    gst_element_sync_state_with_parent(appsink);

    if (!gst_element_link_many(videoconvert, videorate, videoscale, capsfilter, appsink, nullptr)) {
        LOG_ERROR("GStreamerVideoPlayer: Failed to link video elements.");
        return;
    }

    GstPad* sink_pad = gst_element_get_static_pad(videoconvert, "sink");
    if (sink_pad) {
        if (gst_pad_link(pad, sink_pad) != GST_PAD_LINK_OK) {
            LOG_ERROR("GStreamerVideoPlayer: Failed to link decodebin pad to videoconvert.");
        }
        gst_object_unref(sink_pad);
    }
}

void GStreamerVideoPlayer::linkAudioPad(GstPad* pad) {
    GstElement* audioconvert = gst_element_factory_make("audioconvert", "audioconvert");
    GstElement* volume = gst_element_factory_make("volume", "volume");
    GstElement* audiosink = gst_element_factory_make("autoaudiosink", "audiosink");

    if (!audioconvert || !volume || !audiosink) {
        LOG_ERROR("GStreamerVideoPlayer: Failed to create audio elements. Check if gstreamer1.0-plugins-base and gstreamer1.0-plugins-good are installed.");
        if (audioconvert) gst_object_unref(audioconvert);
        if (volume) gst_object_unref(volume);
        if (audiosink) gst_object_unref(audiosink);
        return;
    }

    gst_bin_add_many(GST_BIN(ctx_->pipeline), audioconvert, volume, audiosink, nullptr);

    ctx_->volume_element = volume;
    ctx_->audiosink_element = GST_ELEMENT(gst_object_ref(audiosink)); // Cast added here

    gst_element_sync_state_with_parent(audioconvert);
    gst_element_sync_state_with_parent(volume);
    gst_element_sync_state_with_parent(audiosink);

    if (!gst_element_link_many(audioconvert, volume, audiosink, nullptr)) {
        LOG_ERROR("GStreamerVideoPlayer: Failed to link audio elements.");
        return;
    }

    GstPad* sink_pad = gst_element_get_static_pad(audioconvert, "sink");
    if (sink_pad) {
        if (gst_pad_link(pad, sink_pad) != GST_PAD_LINK_OK) {
            LOG_ERROR("GStreamerVideoPlayer: Failed to link decodebin pad to audioconvert.");
        }
        gst_object_unref(sink_pad);
    }
}