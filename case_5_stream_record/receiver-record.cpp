#include <gst/gst.h>
#include <iostream>
#include <csignal>

GstElement *pipeline = nullptr;
bool running = true;

// Xử lý Ctrl+C để dừng ghi file .mp4 an toàn
void signal_handler(int) {
    std::cout << "\nStopping receiver and finalizing MP4 file..." << std::endl;
    if (pipeline) {
        gst_element_send_event(pipeline, gst_event_new_eos());
    }
    running = false;
}

int main(int argc, char *argv[]) {
    GstBus *bus;
    GstMessage *msg;
    GError *error = nullptr;

    gst_init(&argc, &argv);
    signal(SIGINT, signal_handler);

    // Pipeline nhận và ghi file .mp4
    const gchar *pipeline_desc =
        "mp4mux name=mux faststart=true ! filesink location=video_received.mp4 "
        "udpsrc port=6000 buffer-size=524288 "
        "caps=\"application/x-rtp,media=video,encoding-name=H264,payload=96\" ! "
        "rtpjitterbuffer latency=50 drop-on-latency=true ! rtph264depay ! tee name=vtee "
        "vtee. ! queue ! avdec_h264 ! videoconvert ! autovideosink sync=false async=false "
        "vtee. ! queue ! h264parse ! mux.video_0 "
        "udpsrc port=6002 buffer-size=262144 "
        "caps=\"application/x-rtp,media=audio,encoding-name=OPUS,payload=97\" ! "
        "rtpjitterbuffer latency=30 drop-on-latency=true ! rtpopusdepay ! tee name=atee "
        "atee. ! queue ! opusdec ! audioconvert ! autoaudiosink sync=false async=false "
        "atee. ! queue ! opusparse ! mux.audio_0";

    pipeline = gst_parse_launch(pipeline_desc, &error);
    if (!pipeline) {
        std::cerr << "Failed to create receiver pipeline: " << error->message << std::endl;
        g_error_free(error);
        return -1;
    }

    gst_element_set_state(pipeline, GST_STATE_PLAYING);
    std::cout << "Receiver started (recording to video_received.mp4)\n";
    std::cout << "Press Ctrl+C to stop safely.\n";

    bus = gst_element_get_bus(pipeline);

    while (running) {
        msg = gst_bus_timed_pop_filtered(
            bus, GST_SECOND,
            (GstMessageType)(GST_MESSAGE_ERROR | GST_MESSAGE_EOS | GST_MESSAGE_STATE_CHANGED));

        if (msg) {
            switch (GST_MESSAGE_TYPE(msg)) {
                case GST_MESSAGE_ERROR: {
                    GError *err;
                    gchar *debug_info;
                    gst_message_parse_error(msg, &err, &debug_info);
                    std::cerr << "GStreamer error: " << err->message << std::endl;
                    g_error_free(err);
                    g_free(debug_info);
                    running = false;
                    break;
                }
                case GST_MESSAGE_EOS:
                    std::cout << "EOS received — MP4 file finalized.\n";
                    running = false;
                    break;
                case GST_MESSAGE_STATE_CHANGED:
                    if (GST_MESSAGE_SRC(msg) == GST_OBJECT(pipeline)) {
                        GstState old_state, new_state, pending_state;
                        gst_message_parse_state_changed(msg, &old_state, &new_state, &pending_state);
                        std::cout << "State: "
                                  << gst_element_state_get_name(old_state)
                                  << " → " << gst_element_state_get_name(new_state)
                                  << std::endl;
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

    std::cout << "Receiver stopped and file saved successfully: video_received.mp4\n";
    return 0;
}
