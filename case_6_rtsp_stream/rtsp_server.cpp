#include "rtsp_server.h"
#include <gst/gst.h>
#include <gst/rtsp-server/rtsp-server.h>
#include <iostream>

void start_rtsp_server(const char* ip, int port) {
    std::cout << "[Server] Initializing GStreamer..." << std::endl;
    gst_init(nullptr, nullptr);
    GMainLoop *loop = g_main_loop_new(NULL, FALSE);

    std::cout << "[Server] Creating RTSP server..." << std::endl;
    GstRTSPServer *server = gst_rtsp_server_new();
    std::string port_str = std::to_string(port);
    gst_rtsp_server_set_service(server, port_str.c_str());

    GstRTSPMountPoints *mounts = gst_rtsp_server_get_mount_points(server);
    GstRTSPMediaFactory *factory = gst_rtsp_media_factory_new();

    // SỬA LỖI: Đã XÓA phần pipeline âm thanh (pulsesrc)
    // Nó không thể chạy trong một systemd service
    std::cout << "[Server] Setting GStreamer pipeline (Video Only)..." << std::endl;
    gst_rtsp_media_factory_set_launch(factory,
        "( "
        "v4l2src device=/dev/video0 ! videoconvert ! video/x-raw,format=I420 ! "
        "x264enc tune=zerolatency bitrate=3000 speed-preset=ultrafast ! "
        "rtph264pay name=pay0 pt=96 "
        ")"
    );

    std::cout << "[Server] Attaching factory to /webcam mount point..." << std::endl;
    gst_rtsp_media_factory_set_shared(factory, TRUE);
    gst_rtsp_mount_points_add_factory(mounts, "/webcam", factory);
    g_object_unref(mounts);
    gst_rtsp_server_attach(server, NULL);

    // SỬA LỖI: Dùng std::endl
    std::cout << "[Server] RTSP server running at: rtsp://" << ip << ":" << port << "/webcam" << std::endl;
    
    std::cout << "[Server] Starting GMainLoop..." << std::endl;
    g_main_loop_run(loop);
}