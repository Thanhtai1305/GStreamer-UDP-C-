#include "case2.h"
#include <csignal>

static GMainLoop *loop = nullptr;

// Ctrl+C handler
static void handle_sigint(int) {
    g_print("\nCtrl+C detected! Stopping receiver (case2)...\n");
    if (loop)
        g_main_loop_quit(loop);
}

// Bus message handler
static gboolean bus_call(GstBus *bus, GstMessage *msg, gpointer data) {
    switch (GST_MESSAGE_TYPE(msg)) {
        case GST_MESSAGE_ERROR: {
            GError *err;
            gchar *debug;
            gst_message_parse_error(msg, &err, &debug);
            g_printerr("Error from %s: %s\n", GST_OBJECT_NAME(msg->src), err->message);
            g_printerr("Debug info: %s\n", debug ? debug : "none");
            g_clear_error(&err);
            g_free(debug);
            break;
        }
        case GST_MESSAGE_EOS:
            g_print("EOS received — keeping receiver alive.\n");
            break;
        default:
            break;
    }
    return TRUE;
}

// Start the receiver pipeline for case 2 (video + audio)
void start_receiver_case2() {
    GstElement *video_pipeline, *audio_pipeline;
    GstBus *bus_video, *bus_audio;
    GError *error = NULL;

    gst_init(nullptr, nullptr);

    // Video pipeline
    const gchar *video_str =
        "udpsrc port=6000 caps=\"application/x-rtp, media=video, encoding-name=H264, payload=96\" "
        "! rtph264depay ! avdec_h264 ! videoconvert ! autovideosink sync=false";
    video_pipeline = gst_parse_launch(video_str, &error);
    if (!video_pipeline) {
        g_printerr("Could not create video pipeline: %s\n", error->message);
        g_error_free(error);
        return;
    }

    // Audio pipeline
    const gchar *audio_str =
        "udpsrc port=6002 caps=\"application/x-rtp, media=audio, encoding-name=OPUS, payload=97\" "
        "! rtpopusdepay ! opusdec ! audioconvert ! autoaudiosink sync=false";
    audio_pipeline = gst_parse_launch(audio_str, &error);
    if (!audio_pipeline) {
        g_printerr("Could not create audio pipeline: %s\n", error->message);
        g_error_free(error);
        gst_object_unref(video_pipeline);
        return;
    }

    // Watch both pipelines
    bus_video = gst_element_get_bus(video_pipeline);
    bus_audio = gst_element_get_bus(audio_pipeline);
    gst_bus_add_watch(bus_video, bus_call, nullptr);
    gst_bus_add_watch(bus_audio, bus_call, nullptr);

    gst_element_set_state(video_pipeline, GST_STATE_PLAYING);
    gst_element_set_state(audio_pipeline, GST_STATE_PLAYING);
    g_print("Receiver (case2: video+audio) is running... Press Ctrl+C to stop.\n");

    loop = g_main_loop_new(NULL, FALSE);
    signal(SIGINT, handle_sigint);
    g_main_loop_run(loop);

    g_print("Cleaning up receiver...\n");
    gst_element_set_state(video_pipeline, GST_STATE_NULL);
    gst_element_set_state(audio_pipeline, GST_STATE_NULL);
    gst_object_unref(video_pipeline);
    gst_object_unref(audio_pipeline);
    g_main_loop_unref(loop);
}
