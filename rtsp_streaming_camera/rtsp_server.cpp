#include <gst/gst.h>
#include <gst/rtsp-server/rtsp-server.h>

int main(int argc, char *argv[]) {
    gst_init(&argc, &argv);

    // Tạo main loop
    GMainLoop *loop = g_main_loop_new(NULL, FALSE);

    // Tạo RTSP server
    GstRTSPServer *server = gst_rtsp_server_new();
    gst_rtsp_server_set_service(server, "8554"); // port 8554

    // Mount point
    GstRTSPMountPoints *mounts = gst_rtsp_server_get_mount_points(server);

    // Tạo factory
    GstRTSPMediaFactory *factory = gst_rtsp_media_factory_new();

    /*
        🧠 Pipeline gồm:
        - Video: từ webcam (/dev/video0)
        - Audio: từ microphone (pulsesrc)
        - Cả hai encode → gói RTP → ghép thành session RTSP
    */
    gst_rtsp_media_factory_set_launch(factory,
        "( "
        "v4l2src device=/dev/video0 ! videoconvert ! "
        "x264enc tune=zerolatency bitrate=3000 speed-preset=ultrafast ! "
        "rtph264pay name=pay0 pt=96 "
        "pulsesrc ! audioconvert ! audioresample ! "
        "opusenc ! rtpopuspay name=pay1 pt=97 "
        ")"
    );

    // Cho phép nhiều client cùng xem
    gst_rtsp_media_factory_set_shared(factory, TRUE);

    // Gắn pipeline vào đường dẫn RTSP
    gst_rtsp_mount_points_add_factory(mounts, "/webcam", factory);
    g_object_unref(mounts);

    // Gắn server vào main context
    gst_rtsp_server_attach(server, NULL);

    g_print("🎥 RTSP server đang chạy tại: rtsp://192.168.15.53:8554/webcam\n");
    g_print("📢 Stream gồm cả video + audio\n");

    // Chạy main loop
    g_main_loop_run(loop);

    return 0;
}
