#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <iomanip>

// Đảm bảo đường dẫn này đúng với máy bạn
#include "mavlink/common/mavlink.h"
#include "/home/thanhtai/UDP_GStreamer_Cplusplus/mavlink/ardupilotmega/mavlink_msg_mount_control.h"

#define BUFFER_LENGTH 2048

int main(int argc, char* argv[]) {
    int listen_port = 14540; // Cổng mặc định để nghe từ SITL

    // Cho phép nhập cổng khác từ dòng lệnh: ./sniffer_tool 14550
    if (argc > 1) {
        listen_port = atoi(argv[1]);
    }

    int sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock < 0) {
        perror("Socket Error");
        return -1;
    }

    struct sockaddr_in locAddr;
    memset(&locAddr, 0, sizeof(locAddr));
    locAddr.sin_family = AF_INET;
    locAddr.sin_addr.s_addr = INADDR_ANY;
    locAddr.sin_port = htons(listen_port);

    if (bind(sock, (struct sockaddr *)&locAddr, sizeof(struct sockaddr)) < 0) {
        perror("Bind Error (Port busy?)");
        std::cout << "Hay tat cac chuong trinh khac dang dung cong " << listen_port << " (nhu ./main)\n";
        close(sock);
        return -1;
    }

    std::cout << "=======================================================\n";
    std::cout << "   MAVLINK SNIFFER TOOL - LISTENING ON PORT " << listen_port << "\n";
    std::cout << "   Waiting for ArduPilot data...\n";
    std::cout << "=======================================================\n";

    uint8_t buf[BUFFER_LENGTH];
    ssize_t recsize;
    socklen_t fromlen;
    struct sockaddr_in gcAddr;

    long long packet_count = 0;

    while (true) {
        recsize = recvfrom(sock, (void *)buf, BUFFER_LENGTH, 0, (struct sockaddr *)&gcAddr, &fromlen);
        if (recsize > 0) {
            mavlink_message_t msg;
            mavlink_status_t status;
            
            for (ssize_t i = 0; i < recsize; ++i) {
                if (mavlink_parse_char(MAVLINK_COMM_0, buf[i], &msg, &status)) {
                    packet_count++;

                    // --- BỘ LỌC: CHỈ IN TIN NHẮN TỪ DRONE (SYSID 1) ---
                    if (msg.sysid == 1) {
                        
                        // In Heartbeat (1Hz)
                        if (msg.msgid == MAVLINK_MSG_ID_HEARTBEAT) {
                            mavlink_heartbeat_t hb;
                            mavlink_msg_heartbeat_decode(&msg, &hb);
                            std::cout << "[HEARTBEAT] Mode: " << (int)hb.custom_mode 
                                      << " | State: " << (int)hb.system_status << "\r" << std::flush;
                        }
                        
                        // In Lệnh CAMERA TRIGGER (QUAN TRỌNG)
                        else if (msg.msgid == MAVLINK_MSG_ID_CAMERA_TRIGGER) { // ID 112
                            std::cout << "\n-------------------------------------------------------\n";
                            std::cout << ">>> [!!!] CAMERA TRIGGER RECEIVED (ID 112) [!!!] <<<\n";
                            std::cout << "-------------------------------------------------------\n";
                        }

                        // In Lệnh COMMAND_LONG (Ví dụ set trigger distance)
                        else if (msg.msgid == MAVLINK_MSG_ID_COMMAND_LONG) {
                            mavlink_command_long_t cmd;
                            mavlink_msg_command_long_decode(&msg, &cmd);
                            std::cout << "\n[COMMAND] ID: " << cmd.command 
                                      << " | Param1: " << cmd.param1 << "\n";
                        }

                        // In Phản hồi lệnh (ACK)
                        else if (msg.msgid == MAVLINK_MSG_ID_COMMAND_ACK) {
                            mavlink_command_ack_t ack;
                            mavlink_msg_command_ack_decode(&msg, &ack);
                            std::cout << "\n[ACK] Command: " << ack.command 
                                      << " | Result: " << (int)ack.result << "\n";
                        }

                        // In Thông báo chữ (Status Text)
                        else if (msg.msgid == MAVLINK_MSG_ID_STATUSTEXT) {
                            mavlink_statustext_t txt;
                            mavlink_msg_statustext_decode(&msg, &txt);
                            std::cout << "\n[TEXT] " << txt.text << "\n";
                        }
                        
                        // In Mount Control (Gimbal)
                        else if (msg.msgid == MAVLINK_MSG_ID_MOUNT_CONTROL) {
                             std::cout << "\n[GIMBAL] Mount Control Received\n";
                        }
                    }
                }
            }
        }
    }
    close(sock);
    return 0;
}