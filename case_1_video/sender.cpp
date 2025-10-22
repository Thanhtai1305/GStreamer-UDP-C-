#include "case1.h"
#include <csignal>

static GMainLoop *loop = nullptr;  // Keep main loop in this file

// Handle Ctrl+C signal
static void handle_sigint(int) {
    g_print("\n Ctrl+C detected! Stopping sender...\n");
    if (loop)
        g_main_loop_quit(loop);
}

// Process messages on the bus
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
            g_print("Video ended — keeping pipeline alive.\n");
            break;
        default:
            break;
    }
    return TRUE;
}

void start_sender() {
    GstElement *pipeline;
    GstBus *bus;
    GError *error = NULL;

    gst_init(nullptr, nullptr);

    const gchar *pipeline_str =
        "filesrc location=/home/thanhtai/Downloads/video-sample.mp4 "
        "! decodebin ! videoconvert ! x264enc tune=zerolatency "
        "! rtph264pay ! udpsink host=192.168.15.53 port=6000";

    pipeline = gst_parse_launch(pipeline_str, &error);
    if (!pipeline) {
        g_printerr("Could not build pipeline: %s\n", error->message);
        g_error_free(error);
        return;
    }

    bus = gst_element_get_bus(pipeline);
    gst_bus_add_watch(bus, bus_call, nullptr);

    gst_element_set_state(pipeline, GST_STATE_PLAYING);
    g_print("Pipeline Sender is running (Press Ctrl+C to stop)...\n");

    loop = g_main_loop_new(NULL, FALSE);
    signal(SIGINT, handle_sigint);
    g_main_loop_run(loop);

    g_print("Cleaning up sender...\n");
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(bus);
    gst_object_unref(pipeline);
    g_main_loop_unref(loop);
}
