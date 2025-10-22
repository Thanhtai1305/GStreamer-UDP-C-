#include <gst/gst.h>
#include <glib.h>

int main(int argc, char *argv[]) {
    GstElement *video_pipeline, *audio_pipeline;
    GError *error = NULL;
    GMainLoop *loop;

    gst_init(&argc, &argv);

    // 1. Create video pipeline from webcam
    // Ensure to replace /dev/video0 with your webcam device if different
    const gchar *video_str = "v4l2src device=/dev/video0 ! videoconvert ! x264enc tune=zerolatency bitrate=500 speed-preset=superfast ! rtph264pay pt=96 ! udpsink host=192.168.15.53 port=6000";
    video_pipeline = gst_parse_launch(video_str, &error);
    if (!video_pipeline) {
        g_printerr("Could not create video pipeline: %s\n", error->message);
        g_error_free(error);
        return -1;
    }

    // 2. Create audio pipeline from microphone
    const gchar *audio_str = "pulsesrc ! queue ! audioconvert ! audioresample ! opusenc ! rtpopuspay pt=97 ! udpsink host=192.168.15.53 port=6002";
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
    g_print("Sender pipelines are running (webcam and mic)...\n");

    // 4. Create main loop to keep the program running
    // Since this is a live source stream, it will not end by itself (EOS).
    loop = g_main_loop_new(NULL, FALSE);
    g_main_loop_run(loop);

    // 5. Cleanup resources (Press Ctrl+C to exit)
    g_print("Dừng các pipeline...\n");
    gst_element_set_state(video_pipeline, GST_STATE_NULL);
    gst_element_set_state(audio_pipeline, GST_STATE_NULL);
    gst_object_unref(video_pipeline);
    gst_object_unref(audio_pipeline);
    g_main_loop_unref(loop);

    return 0;
}