#include <gst/gst.h>
#include <glib.h>


// IP address of the machine you want to SEND the stream to
#define REMOTE_IP "192.168.15.53" 

// Port you use to SEND video and audio
#define SENDER_VIDEO_PORT 7000
#define SENDER_AUDIO_PORT 7002

// Port you use to RECEIVE video and audio
#define RECEIVER_VIDEO_PORT 6000
#define RECEIVER_AUDIO_PORT 6002



int main(int argc, char *argv[]) {
    GstElement *p_send_vid, *p_send_aud, *p_recv_vid, *p_recv_aud;
    GMainLoop *loop;
    gchar *video_send_str, *audio_send_str, *video_recv_str, *audio_recv_str;

    gst_init(&argc, &argv);

    // 1. Build pipeline strings
    
    // Pipeline to send video from webcam
    video_send_str = g_strdup_printf(
        "videotestsrc ! videoconvert ! x264enc tune=zerolatency bitrate=2000 speed-preset=superfast ! "
        "rtph264pay pt=96 ! udpsink host=%s port=%d", REMOTE_IP, SENDER_VIDEO_PORT
    );

    // Pipeline to send audio from microphone
    audio_send_str = g_strdup_printf(
        "pulsesrc ! queue ! audioconvert ! audioresample ! opusenc bitrate=64000 ! "
        "rtpopuspay pt=97 ! udpsink host=%s port=%d", REMOTE_IP, SENDER_AUDIO_PORT
    );

    // Pipeline to receive video
    video_recv_str = g_strdup_printf(
        "udpsrc port=%d caps=\"application/x-rtp, media=video, encoding-name=H264, payload=96\" ! "
        "rtph264depay ! avdec_h264 ! videoconvert ! autovideosink sync=false", RECEIVER_VIDEO_PORT
    );

    // Pipeline to receive audio
    audio_recv_str = g_strdup_printf(
        "udpsrc port=%d caps=\"application/x-rtp, media=audio, encoding-name=OPUS, payload=97\" ! "
        "rtpopusdepay ! opusdec ! audioconvert ! autoaudiosink sync=false", RECEIVER_AUDIO_PORT
    );

    g_print("Pipeline to send video: %s\n", video_send_str);
    g_print("Pipeline to send audio: %s\n", audio_send_str);
    g_print("Pipeline to receive video: %s\n", video_recv_str);
    g_print("Pipeline to receive audio: %s\n", audio_recv_str);

    // 2. Create pipelines from strings
    p_send_vid = gst_parse_launch(video_send_str, NULL);
    p_send_aud = gst_parse_launch(audio_send_str, NULL);
    p_recv_vid = gst_parse_launch(video_recv_str, NULL);
    p_recv_aud = gst_parse_launch(audio_recv_str, NULL);

    if (!p_send_vid || !p_send_aud || !p_recv_vid || !p_recv_aud) {
        g_printerr("Failed to create one or more pipelines.\n");
        return -1;
    }

    // 3. Run all pipelines
    gst_element_set_state(p_send_vid, GST_STATE_PLAYING);
    gst_element_set_state(p_send_aud, GST_STATE_PLAYING);
    gst_element_set_state(p_recv_vid, GST_STATE_PLAYING);
    gst_element_set_state(p_recv_aud, GST_STATE_PLAYING);

    g_print("All video call pipelines are running...\n");

    // 4. Create main loop to keep the program running
    loop = g_main_loop_new(NULL, FALSE);
    g_main_loop_run(loop);

    // 5. Cleanup resources
    g_print("Stopping pipelines...\n");
    gst_element_set_state(p_send_vid, GST_STATE_NULL);
    gst_element_set_state(p_send_aud, GST_STATE_NULL);
    gst_element_set_state(p_recv_vid, GST_STATE_NULL);
    gst_element_set_state(p_recv_aud, GST_STATE_NULL);

    gst_object_unref(p_send_vid);
    gst_object_unref(p_send_aud);
    gst_object_unref(p_recv_vid);
    gst_object_unref(p_recv_aud);
    g_main_loop_unref(loop);

    g_free(video_send_str);
    g_free(audio_send_str);
    g_free(video_recv_str);
    g_free(audio_recv_str);

    return 0;
}