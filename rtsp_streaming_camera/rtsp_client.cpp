#include <gst/gst.h>

int main(int argc, char *argv[]) {
    gst_init(&argc, &argv);

    if (argc < 2) {
        g_printerr("Usage: %<s RTSP_URL>\nExample: %s rtsp://192.168.15.60:8554/webcam\n", argv[0], argv[0]);
        return -1;
    }

    const gchar *rtsp_url = argv[1];
    GError *error = NULL;

    // Pipeline tương đương với lệnh gst-launch-1.0
    gchar *pipeline_str = g_strdup_printf(
        "rtspsrc location=%s latency=50 name=src "
        "src. ! queue ! decodebin ! audioconvert ! audioresample ! autoaudiosink sync=false "
        "src. ! queue ! decodebin ! videoconvert ! autovideosink sync=false",
        rtsp_url
    );

    GstElement *pipeline = gst_parse_launch(pipeline_str, &error);
    g_free(pipeline_str);

    if (!pipeline) {
        g_printerr("Failed to create pipeline: %s\n", error ? error->message : "Unknown error");
        if (error) g_error_free(error);
        return -1;
    }

    // Chạy pipeline
    gst_element_set_state(pipeline, GST_STATE_PLAYING);
    g_print("RTSP Client is playing stream from: %s\n", rtsp_url);

    // Main loop để duy trì client
    GMainLoop *loop = g_main_loop_new(NULL, FALSE);
    g_main_loop_run(loop);

    // Cleanup
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);
    g_main_loop_unref(loop);

    return 0;
}
