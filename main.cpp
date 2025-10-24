#include <iostream>
#include "case_6_rtsp_stream/rtsp_server.h"
#include "case_6_rtsp_stream/rtsp_client.h"

int main() {
    int mode;
    std::cout << "=============================\n";
    std::cout << "TSP Streaming (Case 6)\n";
    std::cout << "=============================\n";
    std::cout << "1. Run RTSP Server\n";
    std::cout << "2. Run RTSP Client\n";
    std::cout << "Choose option: ";
    std::cin >> mode;

    if (mode == 1) {
        std::string ip = "192.168.15.60";
        int port = 8554;
        start_rtsp_server(ip.c_str(), port);
    } else if (mode == 2) {
        std::string url;
        std::cout << "Enter RTSP URL (e.g. rtsp://192.168.15.60:8554/webcam): ";
        std::cin >> url;
        start_rtsp_client(url.c_str());
    } else {
        std::cout << "Invalid choice!\n";
    }

    return 0;
}
