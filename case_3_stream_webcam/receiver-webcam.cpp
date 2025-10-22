#include <gst/gst.h>
#include <glib.h>

int main(int argc, char *argv[]) {
    GstElement *video_pipeline, *audio_pipeline;
    GError *error = NULL;
    GMainLoop *loop;

    gst_init(&argc, &argv);

    // 1. Create video pipeline receiver
    const gchar *video_str = "udpsrc port=6000 caps=\"application/x-rtp, media=video, encoding-name=H264, payload=96\" ! rtph264depay ! avdec_h264 ! videoconvert ! autovideosink sync=false";
    video_pipeline = gst_parse_launch(video_str, &error);
    if (!video_pipeline) {
        g_printerr("Could not create video pipeline: %s\n", error->message);
        g_error_free(error);
        return -1;
    }

    // 2. Create audio pipeline receiver
    const gchar *audio_str = "udpsrc port=6002 caps=\"application/x-rtp, media=audio, encoding-name=OPUS, payload=97\" ! rtpopusdepay ! opusdec ! audioconvert ! autoaudiosink sync=false";
    audio_pipeline = gst_parse_launch(audio_str, &error);
    if (!audio_pipeline) {
        g_printerr("Could not create audio pipeline: %s\n", error->message);
        g_error_free(error);
        gst_object_unref(video_pipeline);
        return -1;
    }

    // 3. Run both pipelines
    gst_element_set_state(video_pipeline, GST_STATE_PLAYING);
    gst_element_set_state(audio_pipeline, GST_STATE_PLAYING);
    g_print("Receiver pipelines are running...\n");

    // 4. Create main loop to keep the program running
    loop = g_main_loop_new(NULL, FALSE);
    g_main_loop_run(loop);

    // 5. Cleanup resources (Press Ctrl+C to exit)
    g_print("Stopping pipelines...\n");
    gst_element_set_state(video_pipeline, GST_STATE_NULL);
    gst_element_set_state(audio_pipeline, GST_STATE_NULL);
    gst_object_unref(video_pipeline);
    gst_object_unref(audio_pipeline);
    g_main_loop_unref(loop);

    return 0;
}