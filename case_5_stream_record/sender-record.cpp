#include "record.h"
#include <iostream>

int start_sender_case5(const char *remote_ip) {
    GstElement *pipeline;
    GstBus *bus;
    GstMessage *msg;
    GError *error = nullptr;

    // Initialize GStreamer
    gst_init(NULL, NULL);

    // Pipeline description for sender
    gchar *pipeline_desc = g_strdup_printf(
        "v4l2src device=/dev/video0 do-timestamp=true ! videoconvert ! "
        "x264enc tune=zerolatency bitrate=4000 speed-preset=ultrafast key-int-max=15 ! "
        "rtph264pay pt=96 config-interval=1 ! "
        "udpsink host=%s port=%d sync=false async=false "
        "pulsesrc do-timestamp=true ! queue ! audioconvert ! audioresample ! "
        "opusenc frame-size=5 bitrate=128000 ! rtpopuspay pt=97 ! "
        "udpsink host=%s port=%d sync=false async=false",
        remote_ip, VIDEO_PORT, remote_ip, AUDIO_PORT
    );

    pipeline = gst_parse_launch(pipeline_desc, &error);
    g_free(pipeline_desc);

    if (!pipeline) {
        std::cerr << "Failed to create sender pipeline: " << error->message << std::endl;
        g_error_free(error);
        return -1;
    }

    gst_element_set_state(pipeline, GST_STATE_PLAYING);
    std::cout << "Sender pipeline started..." << std::endl;

    bus = gst_element_get_bus(pipeline);
    bool terminate = false;

    while (!terminate) {
        msg = gst_bus_timed_pop_filtered(
            bus, GST_CLOCK_TIME_NONE,
            (GstMessageType)(GST_MESSAGE_ERROR | GST_MESSAGE_EOS | GST_MESSAGE_STATE_CHANGED));

        if (msg != nullptr) {
            switch (GST_MESSAGE_TYPE(msg)) {
                case GST_MESSAGE_ERROR: {
                    GError *err;
                    gchar *debug_info;
                    gst_message_parse_error(msg, &err, &debug_info);
                    std::cerr << "GStreamer error: " << err->message << std::endl;
                    g_error_free(err);
                    g_free(debug_info);
                    terminate = true;
                    break;
                }
                case GST_MESSAGE_EOS:
                    std::cout << "EOS received (end of stream)." << std::endl;
                    terminate = true;
                    break;
                case GST_MESSAGE_STATE_CHANGED:
                    if (GST_MESSAGE_SRC(msg) == GST_OBJECT(pipeline)) {
                        GstState old_state, new_state, pending_state;
                        gst_message_parse_state_changed(msg, &old_state, &new_state, &pending_state);
                        std::cout << "State: " 
                                  << gst_element_state_get_name(old_state)
                                  << " → " << gst_element_state_get_name(new_state) << std::endl;
                    }
                    break;
                default:
                    break;
            }
            gst_message_unref(msg);
        }
    }

    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(bus);
    gst_object_unref(pipeline);
    std::cout << "Sender pipeline stopped." << std::endl;

    return 0;
}