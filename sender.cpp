#include <iostream>
#include <unistd.h>
#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <vector>
#include <thread>
#include <atomic>
#include <string> // Cần cho std::string và std::getline

#include <sodium.h>
#include "mavlink/common/mavlink.h"

#define TARGET_IP "127.0.0.1"
#define TARGET_PORT 14550

const unsigned char MY_KEY[crypto_secretbox_KEYBYTES] = {
    0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88,
    0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x00,
    0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88,
    0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x00
};

// Biến cờ
std::atomic<bool> is_connected(false); 
std::atomic<bool> ack_received(false); 

// Hàm gửi
void send_encrypted_mavlink(int sockfd, struct sockaddr_in& dest_addr, mavlink_message_t& msg) {
    uint8_t plain_buf[MAVLINK_MAX_PACKET_LEN];
    uint16_t plain_len = mavlink_msg_to_send_buffer(plain_buf, &msg);

    unsigned char nonce[crypto_secretbox_NONCEBYTES];
    randombytes_buf(nonce, sizeof nonce);

    std::vector<unsigned char> ciphertext(crypto_secretbox_MACBYTES + plain_len);
    crypto_secretbox_easy(ciphertext.data(), plain_buf, plain_len, nonce, MY_KEY);

    std::vector<unsigned char> final_packet;
    final_packet.insert(final_packet.end(), nonce, nonce + sizeof nonce);
    final_packet.insert(final_packet.end(), ciphertext.begin(), ciphertext.end());

    sendto(sockfd, final_packet.data(), final_packet.size(), 0, 
           (struct sockaddr *)&dest_addr, sizeof(dest_addr));
}

// Luồng lắng nghe
void listen_thread_func(int sockfd) {
    uint8_t recv_buf[4096];
    struct sockaddr_in src_addr;
    socklen_t addr_len = sizeof(src_addr);

    while (true) { // Luôn luôn lắng nghe
        ssize_t recv_len = recvfrom(sockfd, recv_buf, 4096, 0, (struct sockaddr*)&src_addr, &addr_len);
        if (recv_len > crypto_secretbox_NONCEBYTES + crypto_secretbox_MACBYTES) {
            unsigned char nonce[crypto_secretbox_NONCEBYTES];
            memcpy(nonce, recv_buf, sizeof nonce);
            unsigned char* ciphertext = recv_buf + sizeof nonce;
            unsigned long long ciphertext_len = recv_len - sizeof nonce;
            std::vector<unsigned char> decrypted(ciphertext_len - crypto_secretbox_MACBYTES);

            if (crypto_secretbox_open_easy(decrypted.data(), ciphertext, ciphertext_len, nonce, MY_KEY) == 0) {
                mavlink_message_t msg;
                mavlink_status_t status;
                for (auto b : decrypted) {
                    if (mavlink_parse_char(MAVLINK_COMM_1, b, &msg, &status)) {
                        
                        if (msg.msgid == MAVLINK_MSG_ID_HEARTBEAT) {
                            if (!is_connected) {
                                std::cout << "\n[System] Da ket noi voi Receiver!" << std::endl;
                                is_connected = true;
                            }
                        } 
                        else if (msg.msgid == MAVLINK_MSG_ID_COMMAND_ACK) {
                            // Khi nhận ACK -> Đánh dấu là tin nhắn đã đến nơi
                            if (!ack_received) {
                                std::cout << "\n[System] -> Receiver da nhan duoc tin nhan!" << std::endl;
                                ack_received = true; 
                            }
                        }
                    }
                }
            }
        }
    }
}

int main() {
    if (sodium_init() < 0) return -1;
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    
    struct sockaddr_in dest_addr;
    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(TARGET_PORT);
    dest_addr.sin_addr.s_addr = inet_addr(TARGET_IP);

    std::thread listener(listen_thread_func, sockfd);
    listener.detach(); // Cho luồng nghe chạy ngầm độc lập

    // 1. Handshake
    std::cout << "[Sender] Dang cho ket noi (Gui Heartbeat)..." << std::endl;
    while (!is_connected) {
        mavlink_message_t msg;
        mavlink_msg_heartbeat_pack(1, 1, &msg, MAV_TYPE_QUADROTOR, MAV_AUTOPILOT_ARDUPILOTMEGA, MAV_MODE_GUIDED_ARMED, 0, MAV_STATE_ACTIVE);
        send_encrypted_mavlink(sockfd, dest_addr, msg);
        usleep(500000); 
    }

    // 2. Vòng lặp Chat
    std::string input_str;
    while (true) {
        std::cout << "\nNhap tin nhan (go 'exit' de thoat): ";
        std::getline(std::cin, input_str); // Nhập chuỗi có khoảng trắng

        if (input_str == "exit") break;
        if (input_str.empty()) continue;

        // Giới hạn độ dài cho MAVLink (STATUSTEXT max 50 chars)
        if (input_str.length() > 50) {
            std::cout << "[Canh bao] Tin nhan qua dai, se bi cat xuong 50 ky tu." << std::endl;
            input_str = input_str.substr(0, 50);
        }

        // Reset cờ ACK để bắt đầu gửi tin mới
        ack_received = false;
        
        // Vòng lặp gửi tin cậy: Gửi mãi cho đến khi nhận được ACK
        int retry_count = 0;
        while (!ack_received) {
            mavlink_message_t msg;
            // Convert string sang const char*
            mavlink_msg_statustext_pack(1, 1, &msg, MAV_SEVERITY_INFO, input_str.c_str(), 0, 0);
            
            send_encrypted_mavlink(sockfd, dest_addr, msg);
            
            if (retry_count == 0) std::cout << "Dang gui... ";
            else std::cout << "." << std::flush; // In dấu chấm mỗi lần thử lại
            
            retry_count++;
            usleep(500000); // 0.5s gửi lại 1 lần nếu chưa thấy ACK
        }
    }

    close(sockfd);
    return 0;
}