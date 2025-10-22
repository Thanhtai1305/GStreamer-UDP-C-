#include "case2.h"
#include <csignal>

static GMainLoop *loop = nullptr;

// Ctrl+C handler
static void handle_sigint(int) {
    g_print("\nCtrl+C detected! Stopping sender (case2)...\n");
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
            g_print("EOS reached — keeping sender alive.\n");
            break;
        default:
            break;
    }
    return TRUE;
}

// Start the sender pipeline for case 2 (video + audio)
void start_sender_case2() {
    GstElement *pipeline;
    GstBus *bus;
    GError *error = NULL;

    gst_init(nullptr, nullptr);

    const gchar *pipeline_str =
        "filesrc location=/home/thanhtai/Downloads/test_video.mp4 ! decodebin name=d "
        "d. ! queue ! videoconvert ! x264enc tune=zerolatency bitrate=2000 speed-preset=superfast "
        "! rtph264pay pt=96 ! udpsink host=192.168.15.53 port=6000 "
        "d. ! queue ! audioconvert ! audioresample ! opusenc "
        "! rtpopuspay pt=97 ! udpsink host=192.168.15.53 port=6002";

    pipeline = gst_parse_launch(pipeline_str, &error);
    if (!pipeline) {
        g_printerr("Could not build pipeline: %s\n", error->message);
        g_error_free(error);
        return;
    }

    bus = gst_element_get_bus(pipeline);
    gst_bus_add_watch(bus, bus_call, nullptr);

    gst_element_set_state(pipeline, GST_STATE_PLAYING);
    g_print("Sender (case2: video+audio) is running... Press Ctrl+C to stop.\n");

    loop = g_main_loop_new(NULL, FALSE);
    signal(SIGINT, handle_sigint);
    g_main_loop_run(loop);

    g_print("Cleaning up sender...\n");
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(bus);
    gst_object_unref(pipeline);
    g_main_loop_unref(loop);
}
