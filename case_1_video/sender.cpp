#include <gst/gst.h>
#include <glib.h>

int main(int argc, char *argv[]) {
    GstElement *pipeline;
    GstBus *bus;
    GstMessage *msg;
    GError *error = NULL;

    // 1. Initialize the GStreamer
    gst_init(&argc, &argv);

    // 2. Build the pipeline from the command line string
    // Change the IP address to the receiver's IP
    const gchar *pipeline_str = "filesrc location=/home/thanhtai/Downloads/video-sample.mp4 ! decodebin ! videoconvert ! x264enc tune=zerolatency ! rtph264pay ! udpsink host=192.168.15.53 port=6000";

    pipeline = gst_parse_launch(pipeline_str, &error);

    // Check for errors in building the pipeline
    if (!pipeline) {
        g_printerr("Could not build the pipeline: %s\n", error->message);
        g_error_free(error);
        return -1;
    }

    // 3. Run the pipeline
    gst_element_set_state(pipeline, GST_STATE_PLAYING);
    g_print("Pipeline Sender is running...\n");

    // 4. Wait until error or end of thread (EOS)
    bus = gst_element_get_bus(pipeline);
    msg = gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE, (GstMessageType)(GST_MESSAGE_ERROR | GST_MESSAGE_EOS));

    // Analyze the message
    if (msg != NULL) {
        switch (GST_MESSAGE_TYPE(msg)) {
            case GST_MESSAGE_ERROR: {
                GError *err;
                gchar *debug_info;
                gst_message_parse_error(msg, &err, &debug_info);
                g_printerr("Error from element %s: %s\n", GST_OBJECT_NAME(msg->src), err->message);
                g_printerr("Debug info: %s\n", debug_info ? debug_info : "none");
                g_clear_error(&err);
                g_free(debug_info);
                break;
            }
            case GST_MESSAGE_EOS:
                g_print("End-Of-Stream reached.\n");
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