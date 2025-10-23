#include <gst/gst.h>
#include <gst/rtsp-server/rtsp-server.h>

int main(int argc, char *argv[]) {
    gst_init(nullptr, nullptr);

    // Create main loop
    GMainLoop *loop = g_main_loop_new(NULL, FALSE);

    // Create RTSP server
    GstRTSPServer *server = gst_rtsp_server_new();
    gst_rtsp_server_set_service(server, "8554"); // port 8554

    // Create mount points (paths to RTSP streams)
    GstRTSPMountPoints *mounts = gst_rtsp_server_get_mount_points(server);

    // Create video factory
    GstRTSPMediaFactory *factory = gst_rtsp_media_factory_new();

    // Pipeline to streaming webcam:
    // v4l2src: read from webcam device
    // videoconvert: convert video format
    // x264enc: encode video to H.264
    // rtph264pay: package into RTP for RTSP
    gst_rtsp_media_factory_set_launch(factory,
        "( v4l2src device=/dev/video0 ! videoconvert ! x264enc tune=zerolatency bitrate=1200 speed-preset=ultrafast ! rtph264pay name=pay0 pt=96 )");

    // Permit multiple clients to access the stream
    gst_rtsp_media_factory_set_shared(factory, TRUE);

    // Mount the factory at the path "/webcam"
    gst_rtsp_mount_points_add_factory(mounts, "/webcam", factory);

    // Unref mount points
    g_object_unref(mounts);

    // Start server
    gst_rtsp_server_attach(server, NULL);
    g_print("RTSP server is running at: rtsp://192.168.15.53:8554/webcam\n");

    // Run main loop
    g_main_loop_run(loop);

    return 0;
}
