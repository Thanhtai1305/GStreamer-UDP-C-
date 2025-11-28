#include <iostream>
#include <vector>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <iomanip> 
#include <thread>
#include <atomic>
#include <mutex>
#include <sstream>
#include <gst/gst.h>
#include <gst/rtsp-server/rtsp-server.h>
#include "mavlink/common/mavlink.h"
#include <signal.h>
#include <sys/wait.h>


static const auto _force_cout_flush = []() {
    std::cout << std::unitbuf;  // In ngay, khong buffer
    std::cerr << std::unitbuf;
    return 0;
}();

// Cau hinh mang
#define QGC_IP "127.0.0.1"
#define QGC_PORT 14550 
#define MY_PORT 14540
#define RTSP_PORT 8554

// IDs
#define SYS_ID 1         
#define COMP_ID_CAMERA 100 

// Macro cho cac commands
#define MAV_CMD_LEGACY_PHOTO 203 
#define MAV_CMD_REQUEST_CAMERA_INFORMATION 521

// Global state
int sock;
struct sockaddr_in qgcAddr;
struct sockaddr_in myAddr;
std::atomic<int> image_count(0);
std::atomic<bool> video_recording(false);
std::atomic<bool> running(true);
std::mutex log_mutex;

// PID cua process quay video (ffmpeg)
pid_t ffmpeg_pid = -1;

uint32_t get_time_boot_ms() { 
    return (uint32_t)time(NULL) * 1000; 
}

// ---VIDEO RECORDING FUNCTIONS---

void start_video_recording() {
    if (ffmpeg_pid > 0) {
        std::cout << "[VIDEO] Dang quay do, khong the bat dau moi!\n";
        return;
    }

    // Tao ten file dua tren thoi gian 
    time_t now = time(0);
    std::stringstream ss;
    ss << "video_" << now << ".mp4";
    std::string filename = ss.str();

    std::cout << "[VIDEO] >> BAT DAU QUAY: " << filename << " <<<\n";

    // Fork process de chay ffmpeg nen
    pid_t pid = fork();

    if (pid == 0) {
        // CHILLD PROCESS
        // Lenh: ffmpeg -y -i rtsp://.... -c copy video.mp4
        // Dung -c copy de chi copy stream va khong encode lai -> nhe cho viec quay video

        int devnull = open("/dev/null", O_WRONLY);
        dup2(devnull, STDOUT_FILENO);
        dup2(devnull, STDERR_FILENO);
        close(devnull);

        std::string rtsp_url = "rtsp://127.0.0.1:" + std::to_string(RTSP_PORT) + "/webcam";

        // Thuc thi ffmpeg
        execlp("ffmpeg", "ffmpeg", 
               "-y", 
               "-i", rtsp_url.c_str(), 
               "-c", "copy", 
               filename.c_str(), 
               NULL);

        // Neu execlp that bai
        exit(1);
    } else if (pid > 0) {
        // PARENT PROCESS
        ffmpeg_pid = pid; // Luu PID de kill sau khi ket thuc
        video_recording = true;
    } else {
        std::cerr << "[VIDEO] Loi fork process\n";
    }
}


void stop_video_recording() {
    if (ffmpeg_pid > 0) {
        std::cout << "[VIDEO] >> DUNG QUAY VIDEO <<<\n";

        // Gui SIGINT (tuong duong voi Ctrl+C) de ffmpeg dong file mp4 dung chuan
        // LUU Y: Khong dung SIGKILL vi file se bi loi (corrupt)
        kill(ffmpeg_pid, SIGINT);

        // Cho process con ket thuc han
        waitpid(ffmpeg_pid, nullptr, 0);

        ffmpeg_pid = -1;
        video_recording = false;
        std::cout << "[VIDEO] FIle da duoc luu an toan.\n";
    } else {
        std::cout << "[VIDEO] Khong co video nao dang quay.\n";
        video_recording = false;
    }
}

// --- RTSP SERVER FUNCTIONS ---

// Ham chup anh tu RTSP stream
void execute_capture(std::string reason) {
    {
        //std::lock_guard<std::mutex> lock(log_mutex);
        std::cout << "\n[CAMERA] >>> CHỤP ẢNH! (Nguồn: " << reason << ") <<<\n";
    }

    time_t now = time(0);
    std::stringstream ss;
    ss << "snapshot_" << now << ".jpg";
    std::string filename = ss.str();

    // Lenh FFmpeg chup anh tu luong RTSP
    std::string cmd = "ffmpeg -y -analyzeduration 0 -probesize 32 -rtsp_transport tcp -i rtsp://127.0.0.1:" 
                    + std::to_string(RTSP_PORT) + "/webcam -frames:v 1 -q:v 2 " 
                    + filename + " > /dev/null 2>&1";
    
    system(cmd.c_str());

    {
        //std::lock_guard<std::mutex> lock(log_mutex);
        std::cout << "[CAMERA] ĐÃ LƯU ẢNH: " << filename << "\n";
    }
    
    image_count++;
}

// Thread chay RTSP server
void rtsp_server_thread() {
    gst_init(nullptr, nullptr);
    
    GMainLoop *loop = g_main_loop_new(NULL, FALSE);
    GstRTSPServer *server = gst_rtsp_server_new();
    gst_rtsp_server_set_service(server, std::to_string(RTSP_PORT).c_str());

    GstRTSPMountPoints *mounts = gst_rtsp_server_get_mount_points(server);
    GstRTSPMediaFactory *factory = gst_rtsp_media_factory_new();

    // Pipeline Webcam
    gst_rtsp_media_factory_set_launch(factory, 
        "( v4l2src device=/dev/video0 ! videoconvert ! video/x-raw,format=I420 ! "
        "x264enc tune=zerolatency bitrate=3000 speed-preset=ultrafast key-int-max=10 ! "
        "rtph264pay name=pay0 pt=96 )");
    
    gst_rtsp_media_factory_set_shared(factory, TRUE);
    gst_rtsp_mount_points_add_factory(mounts, "/webcam", factory);
    
    g_object_unref(mounts);
    gst_rtsp_server_attach(server, NULL);

    {
        //std::lock_guard<std::mutex> lock(log_mutex);
        std::cout << "[RTSP] Server running at: rtsp://127.0.0.1:" << RTSP_PORT << "/webcam\n";
    }
    
    g_main_loop_run(loop);
}

// --- MAVLINK FUNCTIONS ---

void setup_udp() {
    sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
    int flags = fcntl(sock, F_GETFL, 0);
    fcntl(sock, F_SETFL, flags | O_NONBLOCK);

    // Thiet lap dia chi cua chuong trinh nay
    memset(&myAddr, 0, sizeof(myAddr));
    myAddr.sin_family = AF_INET;
    myAddr.sin_addr.s_addr = INADDR_ANY;
    myAddr.sin_port = htons(MY_PORT);

    if (bind(sock, (struct sockaddr *)&myAddr, sizeof(myAddr)) == -1) {
        perror("Bind error"); 
        exit(1);
    }
    
    // Thiet lap dia chi cho QGC
    memset(&qgcAddr, 0, sizeof(qgcAddr));
    qgcAddr.sin_family = AF_INET;
    qgcAddr.sin_addr.s_addr = inet_addr(QGC_IP);
    qgcAddr.sin_port = htons(QGC_PORT);
   
    std::cout << "============================================================" << std::endl;
    std::cout << "   CAMERA PROTOCOL V2 + RTSP SIMULATOR (CAPTURE + RECORDER)" << std::endl;
    std::cout << "============================================================" << std::endl;
    std::cout << "   MAVLink Port: " << MY_PORT << " -> " << QGC_PORT << std::endl;
    std::cout << "   RTSP Port:    " << RTSP_PORT << std::endl;
    std::cout << "============================================================" << std::endl;
}

//Ham gui mavlink
void send_mavlink(mavlink_message_t* msg) {
    uint8_t buf[MAVLINK_MAX_PACKET_LEN];
    uint16_t len = mavlink_msg_to_send_buffer(buf, msg);
    sendto(sock, buf, len, 0, (struct sockaddr*)&qgcAddr, sizeof(qgcAddr));
}

// Ham gui Heartbeat
void send_heartbeat() {
    mavlink_message_t msg;
    mavlink_msg_heartbeat_pack(SYS_ID, COMP_ID_CAMERA, &msg,
                               MAV_TYPE_CAMERA, MAV_AUTOPILOT_INVALID,
                               0, 0, MAV_STATE_ACTIVE);
    send_mavlink(&msg);
}

// Ham gui thong tin cua camera
void send_camera_information() {
    mavlink_message_t msg;
    
    const char* uri = "http://192.168.15.60:8000/camera_definition.xml"; 
    
    uint8_t v[32] = "SimCam";
    uint8_t m[32] = "Virtual Camera V2";
    
    uint32_t flags = CAMERA_CAP_FLAGS_CAPTURE_IMAGE | 
                     CAMERA_CAP_FLAGS_CAPTURE_VIDEO |
                     CAMERA_CAP_FLAGS_HAS_MODES |
                     CAMERA_CAP_FLAGS_HAS_VIDEO_STREAM;

    mavlink_msg_camera_information_pack(
        SYS_ID, COMP_ID_CAMERA, &msg,
        get_time_boot_ms(), v, m, 1, 50.0f, 10.0f, 10.0f, 1920, 1080, 0,
        flags, 
        1,
        uri, 0, 0 
    );
    send_mavlink(&msg);
    
    //std::lock_guard<std::mutex> lock(log_mutex);
    std::cout << " [SEND] CAMERA_INFORMATION\n";
}

// Ham gui cau hinh camera
void send_camera_settings() {
    mavlink_message_t msg;
    mavlink_msg_camera_settings_pack(
        SYS_ID, COMP_ID_CAMERA, &msg,
        get_time_boot_ms(),
        CAMERA_MODE_IMAGE,
        1.0f,
        1.0f,
        0
    );
    send_mavlink(&msg);
    
    //std::lock_guard<std::mutex> lock(log_mutex);
    std::cout << " [SEND] CAMERA_SETTINGS\n";
}

// Ham gui trang thai chup anh
void send_camera_capture_status() {
    mavlink_message_t msg;
    
    uint8_t image_status = 0;  // IDLE
    uint8_t video_status = video_recording ? 1 : 0;  // 1 = CAPTURING
    
    mavlink_msg_camera_capture_status_pack(
        SYS_ID, COMP_ID_CAMERA, &msg,
        get_time_boot_ms(),
        image_status,
        video_status,
        0.0f,
        0,
        27000.0f,
        image_count.load(),
        0
    );
    send_mavlink(&msg);
}

// Ham gui thong tin ve bo nho cho : chup anh, quay video
void send_storage_information() {
    mavlink_message_t msg;
    mavlink_msg_storage_information_pack(
        SYS_ID, COMP_ID_CAMERA, &msg,
        get_time_boot_ms(),
        1, 1, STORAGE_STATUS_READY,
        32000.0f, 5000.0f, 27000.0f,
        90.0f, 45.0f, STORAGE_TYPE_SD,
        "", 0
    );
    send_mavlink(&msg);
    
    //std::lock_guard<std::mutex> lock(log_mutex);
    std::cout << " [SEND] STORAGE_INFORMATION\n";
}

// Ham gui thong tin cua stream video
void send_video_stream_information() {
    mavlink_message_t msg;
    
    char name[32] = "Webcam Stream";
    char uri[160];
    snprintf(uri, sizeof(uri), "rtsp://127.0.0.1:%d/webcam", RTSP_PORT);
    
    mavlink_msg_video_stream_information_pack(
        SYS_ID, COMP_ID_CAMERA, &msg,
        1,  // stream_id
        1,  // count
        VIDEO_STREAM_TYPE_RTSP,
        VIDEO_STREAM_STATUS_FLAGS_RUNNING,
        30.0f, 1920, 1080, 3000,
        0, 60,
        name, uri,
        0, 0
    );
    send_mavlink(&msg);
    
    //std::lock_guard<std::mutex> lock(log_mutex);
    std::cout << " [SEND] VIDEO_STREAM_INFORMATION: " << uri << "\n";
}

// Ham gui ACK xac nhan
void send_ack(uint16_t command, uint8_t result = MAV_RESULT_ACCEPTED) {
    mavlink_message_t msg;
    mavlink_msg_command_ack_pack(SYS_ID, COMP_ID_CAMERA, &msg, 
                                 command, result, 0, 0, 0, 0);
    send_mavlink(&msg);
}

// Ham gui lenh chup anh
void send_image_captured() {
    mavlink_message_t msg;
    float q[4] = {1,0,0,0};
    
    mavlink_msg_camera_image_captured_pack(SYS_ID, COMP_ID_CAMERA, &msg,
        get_time_boot_ms(), 0, 0, 0, 0, 0, 0, q, 
        image_count.load(), 1, "");
    send_mavlink(&msg);
    
    //std::lock_guard<std::mutex> lock(log_mutex);
    std::cout << " [EVENT] CAMERA_IMAGE_CAPTURED #" << image_count.load() << "\n";
}

// Ham dam nhiem xu ly lenh cho he thong
void handle_command_long(mavlink_command_long_t& cmd) {
    //std::lock_guard<std::mutex> lock(log_mutex);
    std::cout << "\n[RECV] Command: " << cmd.command << std::endl;
    
    // MAV_CMD_REQUEST_MESSAGE
    if (cmd.command == MAV_CMD_REQUEST_MESSAGE) {
        uint32_t msg_id = (uint32_t)cmd.param1;
        
        if (msg_id == MAVLINK_MSG_ID_CAMERA_INFORMATION) {
            send_camera_information();
            send_ack(cmd.command);
        }
        else if (msg_id == MAVLINK_MSG_ID_CAMERA_SETTINGS) {
            send_camera_settings();
            send_ack(cmd.command);
        }
        else if (msg_id == MAVLINK_MSG_ID_CAMERA_CAPTURE_STATUS) {
            send_camera_capture_status();
            send_ack(cmd.command);
        }
        else if (msg_id == MAVLINK_MSG_ID_STORAGE_INFORMATION) {
            send_storage_information();
            send_ack(cmd.command);
        }
        else if (msg_id == MAVLINK_MSG_ID_VIDEO_STREAM_INFORMATION) {
            send_video_stream_information();
            send_ack(cmd.command);
        }
        else {
            send_ack(cmd.command, MAV_RESULT_UNSUPPORTED);
        }
    }
    
    // MAV_CMD_IMAGE_START_CAPTURE
    else if (cmd.command == MAV_CMD_IMAGE_START_CAPTURE) {
        std::cout << " -> IMAGE_START_CAPTURE\n";
        send_ack(cmd.command);
        
        // Chup anh tu RTSP stream trong thread rieng
        std::thread(execute_capture, "QGC Button").detach();
        
        // Delay mot khoang thoi gian roi gui lenh CAMERA_IMAGE_CAPTURED
        usleep(500000);  // 500ms
        send_image_captured();
    }
    
    // MAV_CMD_IMAGE_STOP_CAPTURE
    else if (cmd.command == MAV_CMD_IMAGE_STOP_CAPTURE) {
        send_ack(cmd.command);
    }
    
    // MAV_CMD_VIDEO_START_CAPTURE
    else if (cmd.command == MAV_CMD_VIDEO_START_CAPTURE) {
        std::cout << " -> VIDEO_START_CAPTURE\n";
        video_recording = true;
        start_video_recording();
        send_ack(cmd.command);
    }
    
    // MAV_CMD_VIDEO_STOP_CAPTURE
    else if (cmd.command == MAV_CMD_VIDEO_STOP_CAPTURE) {
        std::cout << " -> VIDEO_STOP_CAPTURE\n";
        video_recording = false;
        stop_video_recording();
        send_ack(cmd.command);
    }
    
    // MAV_CMD_SET_CAMERA_MODE
    else if (cmd.command == MAV_CMD_SET_CAMERA_MODE) {
        int mode = (int)cmd.param2;
        std::cout << " -> SET_CAMERA_MODE: " << mode << "\n";
        send_ack(cmd.command);
        usleep(100000);
        send_camera_settings();
    }
    
    // Legacy commands
    else if (cmd.command == MAV_CMD_REQUEST_CAMERA_INFORMATION) {
        send_camera_information();
        send_ack(cmd.command);
    }
    else if (cmd.command == MAV_CMD_REQUEST_CAMERA_SETTINGS) {
        send_camera_settings();
        send_ack(cmd.command);
    }
    else if (cmd.command == MAV_CMD_REQUEST_CAMERA_CAPTURE_STATUS) {
        send_camera_capture_status();
        send_ack(cmd.command);
    }
    else if (cmd.command == MAV_CMD_LEGACY_PHOTO) {
        if ((int)cmd.param5 == 1) {
            std::cout << " -> LEGACY PHOTO\n";
            send_ack(cmd.command);
            std::thread(execute_capture, "Legacy Command").detach();
            usleep(500000);
            send_image_captured();
        }
    }
    
    // Camera Trigger (112) - tu Mission
    else if (cmd.command == 112) { //CAMERA_TRIGGER
        std::cout << " -> CAMERA_TRIGGER from Mission\n";
        send_ack(cmd.command);
        std::thread(execute_capture, "Mission Trigger").detach();
        usleep(500000);
        send_image_captured();
    }
    
    else {
        send_ack(cmd.command, MAV_RESULT_UNSUPPORTED);
    }
}

void status_loop() {
    std::cout << "[THREAD STATUS] Status loop started!\n";
    while (running) {
        send_camera_capture_status();
        std::cout << "[THREAD STATUS] CAPTURE_STATUS sent (image_count=" << image_count.load() << ")\n";
        sleep(1);
    }
    std::cout << "[THREAD STATUS] Thread status_loop kết thúc!\n";
}

// Ham gui heartbeat lien tuc : luon cap nhat trang thai cho phia QGC rang: van con hoat dong
void heartbeat_loop() {
    int counter = 0;
    while (running) {
        send_heartbeat();
        if (counter % 5 == 0) {
            std::cout << "[THREAD HB] HEARTBEAT + BROADCAST sent (counter=" << counter << ")\n";
            send_camera_information();
        }
        counter++;
        sleep(1);
    }
    std::cout << "[THREAD HB] Thread heartbeat_loop kết thúc!\n";
}

int main() {
    setup_udp();

    // 1. Khoi dong RTSP server
    std::cout << "\n[INIT] Starting RTSP server on port " << RTSP_PORT << "...\n";
    std::thread rtsp_thread(rtsp_server_thread);
    rtsp_thread.detach();
    sleep(3);  


    // 2. Gui thong tin camera NGAY LAP TUC (truoc khi heartbeat loop)  
    std::cout << "[INIT] Sending initial CAMERA_INFORMATION + SETTINGS...\n";
    send_camera_information();      // ← Quan trong nhat
    send_camera_settings();
    send_storage_information();
    send_video_stream_information();  // ← Cho QGC thay stream RTSP

    // Gui vai heartbeat ban đau de QGC nhan dien comp 100
    std::cout << "[INIT] Sending initial heartbeats...\n";
    for (int i = 0; i < 8; i++) {
        send_heartbeat();
        usleep(150000);
    }

    // 3. Khoi dong 2 thread nen QUAN TRONG 
    std::cout << "[MAIN] Khởi động thread HEARTBEAT...\n";
    std::thread hb_thread(heartbeat_loop);      // Gui heartbeat + broadcast info moi 5s
    std::cout << "[MAIN] Khởi động thread STATUS...\n";
    std::thread status_thread(status_loop);     // Gui CAPTURE_STATUS lien tuc


    // 4. Thong bao thanh cong + huong dan nguoi dung
    std::cout << "\n" << std::string(60, '=') << std::endl;
    std::cout << "   MOCK CAMERA + RTSP SIMULATOR DA HOAT DONG 100%!" << std::endl;
    std::cout << std::string(60, '=') << std::endl;
    std::cout << "   Component ID   : " << COMP_ID_CAMERA << " (Camera)" << std::endl;
    std::cout << "   MAVLink Port   : " << MY_PORT << " → " << QGC_PORT << std::endl;
    std::cout << "   RTSP Stream    : rtsp://127.0.0.1:" << RTSP_PORT << "/webcam" << std::endl;
    std::cout << "   XML Server     : http://YOUR_IP:8000/camera_definition.xml" << std::endl;
    std::cout << std::string(60, '=') << std::endl;
    std::cout << "   Lam ngay:" << std::endl;
    std::cout << "   1. Chay: python3 -m http.server 8000" << std::endl;
    std::cout << "   2. Mo QGC → Click Camera → Thay Settings + Video → THANH CONG!" << std::endl;
    std::cout << std::string(60, '=') << "\n" << std::endl;

    // 5. Main loop: nhan lenh tu QGC
    while (running) {
        uint8_t buf[MAVLINK_MAX_PACKET_LEN];
        struct sockaddr_in src;
        socklen_t len = sizeof(src);

        ssize_t recsize = recvfrom(sock, buf, MAVLINK_MAX_PACKET_LEN, 0,
                                   (struct sockaddr*)&src, &len);

        if (recsize > 0) {
            mavlink_message_t msg;
            mavlink_status_t status;

            for (int i = 0; i < recsize; ++i) {
                if (mavlink_parse_char(MAVLINK_COMM_0, buf[i], &msg, &status)) {
                    if (msg.msgid == MAVLINK_MSG_ID_COMMAND_LONG) {
                        mavlink_command_long_t cmd;
                        mavlink_msg_command_long_decode(&msg, &cmd);

                        // XU LY DUNG target_system + target_component
                        if ((cmd.target_system == SYS_ID || cmd.target_system == 0) &&
                            (cmd.target_component == COMP_ID_CAMERA || cmd.target_component == 0)) {
                            handle_command_long(cmd);
                        }
                    }
                    else if (msg.msgid == MAVLINK_MSG_ID_CAMERA_TRIGGER) {
                        //std::lock_guard<std::mutex> lock(log_mutex);
                        std::cout << "\n[RECV] CAMERA_TRIGGER from Mission Planner!\n";
                        std::thread(execute_capture, "Mission Trigger").detach();
                        usleep(500000);
                        send_image_captured();
                    }
                }
            }
        }
        usleep(5000);  // ~200Hz loop
    }

    // 6. Don dep sau khi thoat
    std::cout << "\nShutting down...\n";
    if (ffmpeg_pid > 0) kill(ffmpeg_pid, SIGINT);
    running = false;
    hb_thread.join();
    status_thread.join();
    close(sock);

    return 0;
}