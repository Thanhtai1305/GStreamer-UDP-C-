#include "video_call.h"
#include <string.h>
#include <iostream>

int run_video_call_pipelines(const char *remote_ip, const char *role) {
    GstElement *p_send_vid, *p_send_aud, *p_recv_vid, *p_recv_aud;
    GMainLoop *loop;
    gchar *video_send_str, *audio_send_str, *video_recv_str, *audio_recv_str;
    int sender_video_port, sender_audio_port, receiver_video_port, receiver_audio_port;

    // Chọn cổng dựa trên role
    if (strcmp(role, "A") == 0) {
        sender_video_port = CONFIG_A_SENDER_VIDEO_PORT;
        sender_audio_port = CONFIG_A_SENDER_AUDIO_PORT;
        receiver_video_port = CONFIG_A_RECEIVER_VIDEO_PORT;
        receiver_audio_port = CONFIG_A_RECEIVER_AUDIO_PORT;
    } else if (strcmp(role, "B") == 0) {
        sender_video_port = CONFIG_B_SENDER_VIDEO_PORT;
        sender_audio_port = CONFIG_B_SENDER_AUDIO_PORT;
        receiver_video_port = CONFIG_B_RECEIVER_VIDEO_PORT;
        receiver_audio_port = CONFIG_B_RECEIVER_AUDIO_PORT;
    } else {
        std::cerr << "Invalid role: " << role << ". Must be 'A' or 'B'.\n";
        return -1;
    }

    // Initialize GStreamer
    gst_init(NULL, NULL);

    // Build pipeline strings
    video_send_str = g_strdup_printf(
        "videotestsrc ! videoconvert ! x264enc tune=zerolatency bitrate=2000 speed-preset=superfast ! "
        "rtph264pay pt=96 ! udpsink host=%s port=%d", remote_ip, sender_video_port
    );

    audio_send_str = g_strdup_printf(
        "pulsesrc ! queue ! audioconvert ! audioresample ! opusenc bitrate=64000 ! "
        "rtpopuspay pt=97 ! udpsink host=%s port=%d", remote_ip, sender_audio_port
    );

    video_recv_str = g_strdup_printf(
        "udpsrc port=%d caps=\"application/x-rtp, media=video, encoding-name=H264, payload=96\" ! "
        "rtph264depay ! avdec_h264 ! videoconvert ! autovideosink sync=false", receiver_video_port
    );

    audio_recv_str = g_strdup_printf(
        "udpsrc port=%d caps=\"application/x-rtp, media=audio, encoding-name=OPUS, payload=97\" ! "
        "rtpopusdepay ! opusdec ! audioconvert ! autoaudiosink sync=false", receiver_audio_port
    );

    g_print("Pipeline to send video: %s\n", video_send_str);
    g_print("Pipeline to send audio: %s\n", audio_send_str);
    g_print("Pipeline to receive video: %s\n", video_recv_str);
    g_print("Pipeline to receive audio: %s\n", audio_recv_str);

    // Create pipelines from strings
    p_send_vid = gst_parse_launch(video_send_str, NULL);
    p_send_aud = gst_parse_launch(audio_send_str, NULL);
    p_recv_vid = gst_parse_launch(video_recv_str, NULL);
    p_recv_aud = gst_parse_launch(audio_recv_str, NULL);

    if (!p_send_vid || !p_send_aud || !p_recv_vid || !p_recv_aud) {
        g_printerr("Failed to create one or more pipelines.\n");
        g_free(video_send_str);
        g_free(audio_send_str);
        g_free(video_recv_str);
        g_free(audio_recv_str);
        return -1;
    }

    // Run all pipelines
    gst_element_set_state(p_send_vid, GST_STATE_PLAYING);
    gst_element_set_state(p_send_aud, GST_STATE_PLAYING);
    gst_element_set_state(p_recv_vid, GST_STATE_PLAYING);
    gst_element_set_state(p_recv_aud, GST_STATE_PLAYING);

    g_print("All video call pipelines are running...\n");

    // Create main loop to keep the program running
    loop = g_main_loop_new(NULL, FALSE);
    g_main_loop_run(loop);

    // Cleanup resources
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