#include <iostream>
#include <unistd.h>
#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <vector>
#include <thread>   // Cần cho đa luồng
#include <atomic>   // Cần cho biến cờ hiệu

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

// Biến cờ để kiểm soát trạng thái
std::atomic<bool> is_connected(false); // Đã nhận được Heartbeat phản hồi chưa?
std::atomic<bool> ack_received(false); // Đã nhận được ACK cho chuỗi chưa?

// Hàm gửi gói tin mã hóa (Helper function)
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

// --- LUỒNG LẮNG NGHE (LISTENER THREAD) ---
// Nhiệm vụ: Ngồi chờ xem Receiver có trả lời gì không
void listen_thread_func(int sockfd) {
    uint8_t recv_buf[4096];
    struct sockaddr_in src_addr;
    socklen_t addr_len = sizeof(src_addr);

    while (!ack_received) { // Chạy cho đến khi nhận được ACK thì thôi
        ssize_t recv_len = recvfrom(sockfd, recv_buf, 4096, 0, (struct sockaddr*)&src_addr, &addr_len);
        if (recv_len > crypto_secretbox_NONCEBYTES + crypto_secretbox_MACBYTES) {
            // Giải mã
            unsigned char nonce[crypto_secretbox_NONCEBYTES];
            memcpy(nonce, recv_buf, sizeof nonce);
            unsigned char* ciphertext = recv_buf + sizeof nonce;
            unsigned long long ciphertext_len = recv_len - sizeof nonce;
            std::vector<unsigned char> decrypted(ciphertext_len - crypto_secretbox_MACBYTES);

            if (crypto_secretbox_open_easy(decrypted.data(), ciphertext, ciphertext_len, nonce, MY_KEY) == 0) {
                mavlink_message_t msg;
                mavlink_status_t status;
                for (auto b : decrypted) {
                    if (mavlink_parse_char(MAVLINK_COMM_1, b, &msg, &status)) { // Dùng COMM_1 để tránh lẫn lộn
                        
                        if (msg.msgid == MAVLINK_MSG_ID_HEARTBEAT) {
                            if (!is_connected) {
                                std::cout << "[Sender Listener] Nhan duoc Heartbeat phan hoi tu Receiver! KET NOI OK." << std::endl;
                                is_connected = true;
                            }
                        } 
                        else if (msg.msgid == MAVLINK_MSG_ID_COMMAND_ACK) {
                            std::cout << "[Sender Listener] Nhan duoc ACK! Receiver da nhan duoc chuoi." << std::endl;
                            ack_received = true;
                        }
                    }
                }
            }
        }
    }
}

// --- MAIN THREAD (SENDER) ---
int main() {
    if (sodium_init() < 0) return -1;
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    
    struct sockaddr_in dest_addr;
    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(TARGET_PORT);
    dest_addr.sin_addr.s_addr = inet_addr(TARGET_IP);

    // Bật luồng lắng nghe lên chạy song song
    std::thread listener(listen_thread_func, sockfd);

    // ---------------- GIAI ĐOẠN 1: GỬI HEARTBEAT & CHỜ KẾT NỐI ----------------
    std::cout << "[Sender] Bat dau gui Heartbeat va cho phan hoi..." << std::endl;
    
    while (!is_connected) {
        mavlink_message_t msg;
        mavlink_msg_heartbeat_pack(1, 1, &msg, MAV_TYPE_QUADROTOR, MAV_AUTOPILOT_ARDUPILOTMEGA, MAV_MODE_GUIDED_ARMED, 0, MAV_STATE_ACTIVE);
        send_encrypted_mavlink(sockfd, dest_addr, msg);
        
        usleep(500000); // 0.5 giây gửi 1 lần
    }

    // ---------------- GIAI ĐOẠN 2: GỬI DỮ LIỆU CHUỖI ----------------
    std::cout << "\n[Sender] Da ket noi! Bat dau gui du lieu: abcdefHCMUTE2025" << std::endl;
    
    // Gửi chuỗi cho đến khi nhận được ACK
    while (!ack_received) {
        mavlink_message_t msg;
        // Đóng gói chuỗi vào tin nhắn STATUSTEXT
        mavlink_msg_statustext_pack(1, 1, &msg, MAV_SEVERITY_INFO, "abcdefHCMUTE2025", 0, 0);
        
        send_encrypted_mavlink(sockfd, dest_addr, msg);
        std::cout << "[Sender] Da gui chuoi... cho ACK..." << std::endl;
        
        sleep(1); // 1 giây gửi lại 1 lần nếu chưa thấy ACK
    }

    std::cout << "\n[Sender] HOAN TAT QUA TRINH! Ket thuc chuong trinh." << std::endl;
    
    // Đợi luồng nghe kết thúc
    listener.join();
    close(sockfd);
    return 0;
}