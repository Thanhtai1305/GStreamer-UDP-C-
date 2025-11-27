#include "rtsp_server.h"
#include <gst/gst.h>
#include <gst/rtsp-server/rtsp-server.h>
#include <iostream>
#include <thread>
#include <vector>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <ctime> 
#include <sstream>
#include <chrono>
#include <mutex>

#include "../mavlink/common/mavlink.h"

std::mutex log_mutex;

// --- HÀM CHỤP ẢNH ---
void execute_capture(std::string reason) {
    {
        std::lock_guard<std::mutex> lock(log_mutex);
        std::cout << "\n[CAMERA] >>> KICH HOAT CHUP ANH! (Nguon: " << reason << ") <<<\n";
    }

    time_t now = time(0);
    std::stringstream ss;
    ss << "snapshot_" << now << ".jpg";
    std::string filename = ss.str();

    // Lệnh FFmpeg chụp ảnh từ luồng RTSP
    std::string cmd = "ffmpeg -y -analyzeduration 0 -probesize 32 -rtsp_transport tcp -i rtsp://127.0.0.1:8554/webcam -frames:v 1 -q:v 2 " + filename + " > /dev/null 2>&1";
    
    system(cmd.c_str());

    {
        std::lock_guard<std::mutex> lock(log_mutex);
        std::cout << "[CAMERA] DA LUU ANH: build/" << filename << "\n";
    }
}

// --- LUỒNG LẮNG NGHE MAVLINK ---
void mavlink_listener_thread(int listen_port) {
    int sockfd;
    struct sockaddr_in servaddr, qgc_addr;
    
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) return;

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(listen_port);

    // Set timeout
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 10000; // 10ms
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    if (bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("MAVLink Bind Failed");
        return;
    }

    // Địa chỉ đích (SITL/QGC ở cổng 14550)
    memset(&qgc_addr, 0, sizeof(qgc_addr));
    qgc_addr.sin_family = AF_INET;
    qgc_addr.sin_port = htons(14550); 
    qgc_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    std::cout << "[MAVLink] Ready. Dang YEU CAU DU LIEU tu Drone...\n";

    uint8_t buffer[2048];
    auto last_heartbeat = std::chrono::steady_clock::now();
    auto last_request = std::chrono::steady_clock::now();

    while (true) {
        auto now = std::chrono::steady_clock::now();

        // 1. Gửi Heartbeat (1Hz)
        if (std::chrono::duration_cast<std::chrono::milliseconds>(now - last_heartbeat).count() >= 1000) {
            mavlink_message_t msg_hb;
            uint8_t buf_hb[MAVLINK_MAX_PACKET_LEN];
            mavlink_msg_heartbeat_pack(1, MAV_COMP_ID_CAMERA, &msg_hb, MAV_TYPE_CAMERA, 0, 0, 0, MAV_STATE_ACTIVE);
            uint16_t len = mavlink_msg_to_send_buffer(buf_hb, &msg_hb);
            sendto(sockfd, (const char *)buf_hb, len, 0, (const struct sockaddr *)&qgc_addr, sizeof(qgc_addr));
            last_heartbeat = now;
        }

        // 2. [QUAN TRỌNG] Gửi yêu cầu DATA STREAM (2 giây/lần)
        // Cái này ép Drone phải gửi dữ liệu Trigger/Status về cho mình
        if (std::chrono::duration_cast<std::chrono::milliseconds>(now - last_request).count() >= 2000) {
            mavlink_message_t msg_req;
            uint8_t buf_req[MAVLINK_MAX_PACKET_LEN];
            
            // REQUEST_DATA_STREAM: Hỏi xin tất cả dữ liệu (MAV_DATA_STREAM_ALL)
            // Tần số 10Hz, Start = 1
            mavlink_msg_request_data_stream_pack(1, MAV_COMP_ID_CAMERA, &msg_req, 
                                               1, 1, MAV_DATA_STREAM_ALL, 10, 1);
            
            uint16_t len = mavlink_msg_to_send_buffer(buf_req, &msg_req);
            sendto(sockfd, (const char *)buf_req, len, 0, (const struct sockaddr *)&qgc_addr, sizeof(qgc_addr));
            
            last_request = now;
            // std::cout << "[DEBUG] Da gui Request Data Stream...\n"; 
        }

        // 3. Nhận dữ liệu
        struct sockaddr_in sender_addr;
        socklen_t sender_len = sizeof(sender_addr);
        int n = recvfrom(sockfd, (char *)buffer, 2048, 0, (struct sockaddr *)&sender_addr, &sender_len);
        
        if (n > 0) {
            mavlink_message_t msg;
            mavlink_status_t status;
            for (int i = 0; i < n; ++i) {
                if (mavlink_parse_char(MAVLINK_COMM_0, buffer[i], &msg, &status)) {
                    
                    // --- BẮT CAMERA TRIGGER (112) ---
                    if (msg.msgid == 112) { // CAMERA_TRIGGER
                        std::thread(execute_capture, "AUTO - MISSION TRIGGER").detach();
                    }
                    
                    // --- BẮT LỆNH THỦ CÔNG (COMMAND LONG) ---
                    else if (msg.msgid == MAVLINK_MSG_ID_COMMAND_LONG) {
                        mavlink_command_long_t cmd;
                        mavlink_msg_command_long_decode(&msg, &cmd);
                        
                        // 2000: Start Capture | 203: Digicam Control
                        if (cmd.command == 2000 || cmd.command == 203) {
                            std::thread(execute_capture, "MANUAL - COMMAND").detach();
                        }
                    }
                    
                    // --- DEBUG: BẮT LỆNH SET TRIGGER DISTANCE (206) ---
                    else if (msg.msgid == MAVLINK_MSG_ID_COMMAND_LONG) {
                         mavlink_command_long_t cmd;
                         mavlink_msg_command_long_decode(&msg, &cmd);
                         if (cmd.command == 206) {
                             std::cout << "[DEBUG] Drone da nhan lenh Auto Distance!\n";
                         }
                    }
                }
            }
        }
        usleep(1000); 
    }
}

void start_rtsp_server(const char* ip, int rtsp_port, int mavlink_port) {
    std::thread mavlink_thread(mavlink_listener_thread, mavlink_port);
    mavlink_thread.detach();

    gst_init(nullptr, nullptr);
    GMainLoop *loop = g_main_loop_new(NULL, FALSE);
    GstRTSPServer *server = gst_rtsp_server_new();
    gst_rtsp_server_set_service(server, std::to_string(rtsp_port).c_str());

    GstRTSPMountPoints *mounts = gst_rtsp_server_get_mount_points(server);
    GstRTSPMediaFactory *factory = gst_rtsp_media_factory_new();

    // Pipeline Webcam tối ưu
    gst_rtsp_media_factory_set_launch(factory, "( v4l2src device=/dev/video0 ! videoconvert ! video/x-raw,format=I420 ! x264enc tune=zerolatency bitrate=3000 speed-preset=ultrafast key-int-max=10 ! rtph264pay name=pay0 pt=96 )");
    
    gst_rtsp_media_factory_set_shared(factory, TRUE);
    gst_rtsp_mount_points_add_factory(mounts, "/webcam", factory);
    
    g_object_unref(mounts);
    gst_rtsp_server_attach(server, NULL);

    std::cout << "RTSP server running at: rtsp://" << ip << ":" << rtsp_port << "/webcam\n";
    g_main_loop_run(loop);
}
