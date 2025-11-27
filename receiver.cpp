#include <iostream>
#include <unistd.h>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <vector>

// Thư viện mã hóa
#include <sodium.h>

// Thư viện MAVLink
#include "mavlink/common/mavlink.h"

#define PORT 14550
#define BUFFER_SIZE 4096 // Tăng lên chút để chứa overhead mã hóa

// KHÓA BÍ MẬT (Phải khớp với Sender)
const unsigned char MY_KEY[crypto_secretbox_KEYBYTES] = {
    0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88,
    0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x00,
    0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88,
    0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x00
};

int main() {
    // 0. Khởi tạo Libsodium
    if (sodium_init() < 0) {
        std::cerr << "Khong the khoi tao Libsodium!" << std::endl;
        return -1;
    }

    // 1. Tạo Socket & Bind
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) { perror("Loi tao socket"); return -1; }

    struct sockaddr_in my_addr;
    memset(&my_addr, 0, sizeof(my_addr));
    my_addr.sin_family = AF_INET;
    my_addr.sin_addr.s_addr = INADDR_ANY;
    my_addr.sin_port = htons(PORT);

    if (bind(sockfd, (struct sockaddr *)&my_addr, sizeof(my_addr)) < 0) {
        perror("Loi Bind"); return -1;
    }

    std::cout << "[Encrypted Receiver] Dang lang nghe tren cong " << PORT << "..." << std::endl;

    uint8_t recv_buf[BUFFER_SIZE];
    struct sockaddr_in src_addr;
    socklen_t addr_len = sizeof(src_addr);

    while (true) {
        // 2. Nhận gói tin mã hóa từ UDP
        ssize_t recv_len = recvfrom(sockfd, recv_buf, BUFFER_SIZE, 0, 
                                    (struct sockaddr *)&src_addr, &addr_len);

        if (recv_len > 0) {
            // -----------------------------------------------------------
            // 3. GIẢI MÃ (Decryption Step)
            // -----------------------------------------------------------
            
            // Kiểm tra độ dài tối thiểu (Nonce + MAC)
            if (recv_len < crypto_secretbox_NONCEBYTES + crypto_secretbox_MACBYTES) {
                std::cerr << "Goi tin qua ngan, bo qua!" << std::endl;
                continue;
            }

            // a. Tách Nonce (24 bytes đầu)
            unsigned char nonce[crypto_secretbox_NONCEBYTES];
            memcpy(nonce, recv_buf, sizeof nonce);

            // b. Tách Ciphertext (Phần còn lại)
            unsigned char* ciphertext = recv_buf + sizeof nonce;
            unsigned long long ciphertext_len = recv_len - sizeof nonce;

            // c. Chuẩn bị buffer chứa Plaintext
            std::vector<unsigned char> decrypted(ciphertext_len - crypto_secretbox_MACBYTES);

            // d. Giải mã
            if (crypto_secretbox_open_easy(decrypted.data(), ciphertext, ciphertext_len, nonce, MY_KEY) != 0) {
                std::cerr << "[CANH BAO] Giai ma that bai! Khoa sai hoac tin bi sua doi!" << std::endl;
                continue; // Bỏ qua gói tin rác này
            }

            // -----------------------------------------------------------
            // 4. Parse MAVLink (Làm việc trên dữ liệu đã giải mã 'decrypted')
            // -----------------------------------------------------------
            mavlink_message_t msg;
            mavlink_status_t status;

            for (size_t i = 0; i < decrypted.size(); ++i) {
                if (mavlink_parse_char(MAVLINK_COMM_0, decrypted[i], &msg, &status)) {
                    
                    std::cout << "[Receiver] Giai ma OK -> MSG ID: " << (int)msg.msgid;
                    
                    if (msg.msgid == MAVLINK_MSG_ID_HEARTBEAT) {
                        mavlink_heartbeat_t hb;
                        mavlink_msg_heartbeat_decode(&msg, &hb);
                        std::cout << " -> Heartbeat (Mode: " << (int)hb.base_mode << ")" << std::endl;
                    } else {
                        std::cout << std::endl;
                    }
                }
            }
        }
    }

    close(sockfd);
    return 0;
}