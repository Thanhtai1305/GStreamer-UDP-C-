#include <iostream>
#include <unistd.h>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <vector>
#include <sodium.h>
#include "mavlink/common/mavlink.h"

#define PORT 14550
#define BUFFER_SIZE 4096

// KHÓA BÍ MẬT
const unsigned char MY_KEY[crypto_secretbox_KEYBYTES] = {
    0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88,
    0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x00,
    0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88,
    0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x00
};

// Hàm phụ trợ: Mã hóa và gửi ngược lại cho Sender
void reply_to_sender(int sockfd, struct sockaddr_in& target_addr, mavlink_message_t& msg) {
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
           (struct sockaddr *)&target_addr, sizeof(target_addr));
}

int main() {
    if (sodium_init() < 0) return -1;

    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in my_addr;
    memset(&my_addr, 0, sizeof(my_addr));
    my_addr.sin_family = AF_INET;
    my_addr.sin_addr.s_addr = INADDR_ANY;
    my_addr.sin_port = htons(PORT);

    if (bind(sockfd, (struct sockaddr *)&my_addr, sizeof(my_addr)) < 0) {
        perror("Loi Bind"); return -1;
    }

    std::cout << "[Receiver] Dang cho ket noi..." << std::endl;

    uint8_t recv_buf[BUFFER_SIZE];
    struct sockaddr_in src_addr; 
    socklen_t addr_len = sizeof(src_addr);

    while (true) {
        // 1. Nhận tin và LƯU ĐỊA CHỈ NGƯỜI GỬI vào src_addr
        ssize_t recv_len = recvfrom(sockfd, recv_buf, BUFFER_SIZE, 0, 
                                    (struct sockaddr *)&src_addr, &addr_len);

        if (recv_len > crypto_secretbox_NONCEBYTES + crypto_secretbox_MACBYTES) {
            // 2. Giải mã
            unsigned char nonce[crypto_secretbox_NONCEBYTES];
            memcpy(nonce, recv_buf, sizeof nonce);
            unsigned char* ciphertext = recv_buf + sizeof nonce;
            unsigned long long ciphertext_len = recv_len - sizeof nonce;
            std::vector<unsigned char> decrypted(ciphertext_len - crypto_secretbox_MACBYTES);

            if (crypto_secretbox_open_easy(decrypted.data(), ciphertext, ciphertext_len, nonce, MY_KEY) != 0) {
                continue;
            }

            // 3. Parse MAVLink
            mavlink_message_t msg;
            mavlink_status_t status;
            for (size_t i = 0; i < decrypted.size(); ++i) {
                if (mavlink_parse_char(MAVLINK_COMM_0, decrypted[i], &msg, &status)) {
                    
                    // --- LOGIC TRẢ LỜI ---
                    
                    if (msg.msgid == MAVLINK_MSG_ID_HEARTBEAT) {
                        // Nhận được Heartbeat -> Gửi phản hồi ngay
                        std::cout << "[Receiver] Nhan Heartbeat tu Sender -> Gui lai Heartbeat." << std::endl;
                        
                        mavlink_message_t reply_msg;
                        // ID 255 là Trạm điều khiển
                        mavlink_msg_heartbeat_pack(255, 0, &reply_msg, MAV_TYPE_GCS, MAV_AUTOPILOT_INVALID, 0, 0, 0);
                        
                        reply_to_sender(sockfd, src_addr, reply_msg);
                    }
                    else if (msg.msgid == MAVLINK_MSG_ID_STATUSTEXT) {
                        // Nhận được chuỗi -> In ra và Gửi ACK
                        mavlink_statustext_t text_msg;
                        mavlink_msg_statustext_decode(&msg, &text_msg);
                        
                        std::cout << "[Receiver] DA NHAN DUOC CHUOI: " << text_msg.text << std::endl;
                        std::cout << "-> Dang gui ACK xac nhan..." << std::endl;

                        mavlink_message_t ack_msg;
                        mavlink_msg_command_ack_pack(255, 0, &ack_msg, 0, MAV_RESULT_ACCEPTED, 0, 0, 0, 0);
                        
                        reply_to_sender(sockfd, src_addr, ack_msg);
                    }
                }
            }
        }
    }
    close(sockfd);
    return 0;
}