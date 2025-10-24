#include "rtsp_client.h"
#include <gst/gst.h>
#include <iostream>

void start_rtsp_client(const char* url) {
    gst_init(nullptr, nullptr);
    GError *error = nullptr;

    std::string pipeline_desc =
        "rtspsrc location=" + std::string(url) + " latency=50 name=src "
        "src. ! queue ! decodebin ! audioconvert ! audioresample ! autoaudiosink sync=false "
        "src. ! queue ! decodebin ! videoconvert ! autovideosink sync=false";

    GstElement *pipeline = gst_parse_launch(pipeline_desc.c_str(), &error);
    if (!pipeline) {
        std::cerr << "Failed to create client pipeline: " << error->message << std::endl;
        g_error_free(error);
        return;
    }

    gst_element_set_state(pipeline, GST_STATE_PLAYING);
    std::cout << "Client connected to " << url << std::endl;

    GMainLoop *loop = g_main_loop_new(NULL, FALSE);
    g_main_loop_run(loop);

    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);
    g_main_loop_unref(loop);
}
