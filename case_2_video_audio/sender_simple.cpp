#include <gst/gst.h>

int main (int argc, char *argv[]) {
    GstElement *pipeline;
    GstBus *bus;
    GstMessage *msg;
    GError *error = NULL;

    gst_init (&argc, &argv);
    
    const gchar *pipeline_str = "filesrc location=/home/thanhtai/Downloads/test_video.mp4 ! decodebin name=d "
                                "d. ! queue ! videoconvert ! x264enc tune=zerolatency bitrate=2000 speed-preset=superfast ! rtph264pay pt=96 ! udpsink host=192.168.15.53 port=6000 "
                                "d. ! queue ! audioconvert ! audioresample ! opusenc ! rtpopuspay pt=97 ! udpsink host=192.168.15.53 port=6002";
    
    pipeline = gst_parse_launch (pipeline_str, &error);

    if(!pipeline) {
        g_printerr(pipeline_str, &error);
        g_error_free(error);
        return -1;
    }

    // Start running the pipeline
    gst_element_set_state(pipeline, GST_STATE_PLAYING);
    g_print("Pipeline Sender (Simple) is running...\n");

    // Wait until error or end of thread (EOS)
    bus = gst_element_get_bus(pipeline);
    msg = gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE, (GstMessageType)(GST_MESSAGE_ERROR | GST_MESSAGE_EOS));
    

    // Free resources
    if (msg != NULL) {
        gst_message_unref(msg);
    }
    gst_object_unref(bus);
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);

    return 0;
}