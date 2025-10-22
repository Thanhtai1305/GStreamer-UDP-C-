#include "case3.h"
#include <csignal>

static GMainLoop *loop = nullptr;

// Handle Ctrl+C signal to stop the main loop
static void handle_sigint(int) {
    g_print("\nCtrl+C detected! Stopping webcam sender...\n");
    if (loop)
        g_main_loop_quit(loop);
}

void start_sender_case3() {
    GstElement *video_pipeline, *audio_pipeline;
    GError *error = NULL;

    gst_init(nullptr, nullptr);

    // Video pipeline (webcam)
    const gchar *video_str =
        "v4l2src device=/dev/video0 ! videoconvert ! "
        "x264enc tune=zerolatency bitrate=5000 speed-preset=superfast ! "
        "rtph264pay pt=96 ! udpsink host=192.168.15.53 port=6000";

    video_pipeline = gst_parse_launch(video_str, &error);
    if (!video_pipeline) {
        g_printerr("Could not create video pipeline: %s\n", error->message);
        g_error_free(error);
        return;
    }

    // Audio pipeline (microphone)
    const gchar *audio_str =
        "pulsesrc ! queue ! audioconvert ! audioresample ! "
        "opusenc ! rtpopuspay pt=97 ! udpsink host=192.168.15.53 port=6002";

    audio_pipeline = gst_parse_launch(audio_str, &error);
    if (!audio_pipeline) {
        g_printerr("Could not create audio pipeline: %s\n", error->message);
        g_error_free(error);
        gst_object_unref(video_pipeline);
        return;
    }

    // Set state and run pipelines
    gst_element_set_state(video_pipeline, GST_STATE_PLAYING);
    gst_element_set_state(audio_pipeline, GST_STATE_PLAYING);
    g_print("Webcam + Mic streaming is running (Press Ctrl+C to stop)...\n");

    loop = g_main_loop_new(NULL, FALSE);
    signal(SIGINT, handle_sigint);
    g_main_loop_run(loop);

    // Cleanup resources
    g_print("Stopping sender pipelines...\n");
    gst_element_set_state(video_pipeline, GST_STATE_NULL);
    gst_element_set_state(audio_pipeline, GST_STATE_NULL);
    gst_object_unref(video_pipeline);
    gst_object_unref(audio_pipeline);
    g_main_loop_unref(loop);
}
