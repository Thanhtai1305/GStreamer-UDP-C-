#ifndef RTSP_SERVER_H
#define RTSP_SERVER_H

// Khai báo hàm start_rtsp_server với 3 tham số:
// 1. ip: Địa chỉ IP (ví dụ "0.0.0.0")
// 2. rtsp_port: Cổng cho video (ví dụ 8554)
// 3. mavlink_port: Cổng để lắng nghe lệnh chụp ảnh (ví dụ 14540)
void start_rtsp_server(const char* ip, int rtsp_port, int mavlink_port);

#endif