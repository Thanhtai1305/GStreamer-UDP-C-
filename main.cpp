#include <iostream>
#include "case_6_rtsp_stream/rtsp_server.h"

int main() {
    int mode;
    std::cout << "=============================\n";
    std::cout << "RTSP Streaming & MAVLink Control\n";
    std::cout << "=============================\n";
    std::cout << "1. Run RTSP Server + MAVLink Listener\n";
    std::cout << "Choose option: ";
    std::cin >> mode;

    if (mode == 1) {
        std::string ip = "0.0.0.0";
        int rtsp_port = 8554;
        
        // QUAN TRỌNG: Dùng cổng 14540 để tránh lỗi 'Bind failed' khi chạy chung với SITL
        // Code sẽ lắng nghe ở 14540 và gửi tin sang 14550 (SITL/QGC)
        int mavlink_port = 14540; 
        
        start_rtsp_server(ip.c_str(), rtsp_port, mavlink_port);
    } else {
        std::cout << "Client mode not ready.\n";
    }

    return 0;
}