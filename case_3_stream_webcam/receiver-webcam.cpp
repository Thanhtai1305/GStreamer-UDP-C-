#include "case3.h"
#include <csignal>

static GMainLoop *loop = nullptr;

// Handle Ctrl+C signal to stop the main loop
static void handle_sigint(int) {
    g_print("\nCtrl+C detected! Stopping webcam receiver...\n");
    if (loop)
        g_main_loop_quit(loop);
}

void start_receiver_case3() {
    GstElement *video_pipeline, *audio_pipeline;
    GError *error = NULL;

    gst_init(nullptr, nullptr);

    // video receiver pipeline
    const gchar *video_str =
        "udpsrc port=6000 caps=\"application/x-rtp, media=video, encoding-name=H264, payload=96\" "
        "! rtph264depay ! avdec_h264 ! videoconvert ! autovideosink sync=false";

    video_pipeline = gst_parse_launch(video_str, &error);
    if (!video_pipeline) {
        g_printerr("Could not create video pipeline: %s\n", error->message);
        g_error_free(error);
        return;
    }

    // Audio receiver pipeline
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

    gst_element_set_state(video_pipeline, GST_STATE_PLAYING);
    gst_element_set_state(audio_pipeline, GST_STATE_PLAYING);
    g_print("Receiver (video + audio) is running (Press Ctrl+C to stop)...\n");

    loop = g_main_loop_new(NULL, FALSE);
    signal(SIGINT, handle_sigint);
    g_main_loop_run(loop);

    // Cleanup resources
    g_print("Stopping receiver pipelines...\n");
    gst_element_set_state(video_pipeline, GST_STATE_NULL);
    gst_element_set_state(audio_pipeline, GST_STATE_NULL);
    gst_object_unref(video_pipeline);
    gst_object_unref(audio_pipeline);
    g_main_loop_unref(loop);
}
