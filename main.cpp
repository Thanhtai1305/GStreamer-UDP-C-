#include <iostream>
#include "case_6_rtsp_stream/rtsp_server.h"
#include "case_6_rtsp_stream/rtsp_client.h"
#include <thread>
#include <chrono>

int main() {
    int mode = 1;
    // Sửa ở đây: dùng std::endl
    std::cout << "=============================" << std::endl;
    std::cout << "TSP Streaming (Case 6)" << std::endl;
    std::cout << "=============================" << std::endl;
    std::cout << "1. Run RTSP Server" << std::endl;
    std::cout << "2. Run RTSP Client" << std::endl;
    std::cout << "Choose option: " << std::endl; // Đổi cả ở đây
   // std::cin >> mode;

    if (mode == 1) {
        std::string ip = "192.168.15.53";
        int port = 8554;
        
        // Thêm log để biết nó đang chạy server
        std::cout << "Mode 1: Starting RTSP Server at " << ip << ":" << port << std::endl;
        
        start_rtsp_server(ip.c_str(), port);
        
         while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(5));
    }
    } else if (mode == 2) {
        std::string url = "rtsp://192.168.15.60:8554/webcam";
        
        // Thêm log
        std::cout << "Mode 2: Starting RTSP Client for URL: " << url << std::endl;
        std::cout << "Enter RTSP URL (e.g. rtsp://192.168.15.53:8554/webcam): ";
      //  std::cin >> url;
        start_rtsp_client(url.c_str());
    } else {
        std::cout << "Invalid choice!" << std::endl; // Sửa ở đây
    }

    return 0;
}