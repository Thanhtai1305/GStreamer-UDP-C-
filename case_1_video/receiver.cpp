#include <gst/gst.h>
#include <glib.h>


int main (int argc, char *argv[]) {
    GstElement *pipeline;
    GstBus *bus;
    GstMessage *msg;
    GError *error = NULL;

    // 1. initialize the GStreamer
    gst_init(&argc, &argv);

    // 2. Build the pipeline from the command line string
    const gchar *pipeline_str = "udpsrc port=6000 ! application/x-rtp, payload=96 ! rtph264depay ! avdec_h264 ! videoconvert ! autovideosink";

    pipeline = gst_parse_launch(pipeline_str, &error);

    // Check for errors in building the pipeline
    if (!pipeline) {
        g_printerr("Could not build the pipeline: %s\n", error->message);
        g_error_free(error);
        return -1;
    }
    
    // 3. Start running the pipeline
    gst_element_set_state(pipeline, GST_STATE_PLAYING);
    g_print("Pipeline Receiver đang chạy...\n");    

    // 4. Wait until error or end of stream (EOS)
    // Since this is a network stream, it will not end by itself (EOS) unless the sender signals it.
    // We will run a main loop and handle messages on the bus.
    bus = gst_element_get_bus(pipeline);
    msg = gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE, (GstMessageType)(GST_MESSAGE_ERROR | GST_MESSAGE_EOS));

    // Analyze the message
    if (msg != NULL) {
        GError *err;
        gchar *debug_info;
        switch (GST_MESSAGE_TYPE(msg)) {
            case GST_MESSAGE_ERROR:
                gst_message_parse_error(msg, &err, &debug_info);
                g_printerr("Error from element %s: %s\n", GST_OBJECT_NAME(msg->src), err->message);
                g_printerr("Debug info: %s\n", debug_info ? debug_info : "không có");
                g_clear_error(&err);
                g_free(debug_info);
                break;
            case GST_MESSAGE_EOS:
                g_print("Reach End-Of-Stream.\n");
                break;
            default:
                // Will not reach here
                g_printerr("Unexpected message received.\n");
                break;
        }
        gst_message_unref(msg);
    }

    // 5. Free resources
    g_print("Stopping pipeline...\n");
    gst_object_unref(bus);
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);

    return 0;
}