#include <iostream>
#include <unistd.h>
#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <vector>

// Thư viện mã hóa
#include <sodium.h>

// Thư viện MAVLink
#include "mavlink/common/mavlink.h"

#define TARGET_IP "127.0.0.1" // có thể thay bằng địa chỉ của máy Nhận
#define TARGET_PORT 14550

// KHÓA BÍ MẬT (32 bytes) - Phải giống hệt bên Receiver
// Trong thực tế nên đọc từ file config an toàn
const unsigned char MY_KEY[crypto_secretbox_KEYBYTES] = {
    0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88,
    0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x00,
    0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88,
    0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x00
};

int main() {
    // 0. Khởi tạo thư viện mã hóa
    if (sodium_init() < 0) {
        std::cerr << "Khong the khoi tao Libsodium!" << std::endl;
        return -1;
    }

    // 1. Tạo UDP Socket
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Loi tao socket");
        return -1;
    }

    struct sockaddr_in dest_addr;
    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(TARGET_PORT);
    dest_addr.sin_addr.s_addr = inet_addr(TARGET_IP);

    std::cout << "[Encrypted Sender] Dang gui MAVLink Heartbeat (Secured)..." << std::endl;

    while (true) {
        // 2. Tạo MAVLink Message
        mavlink_message_t msg;
        mavlink_msg_heartbeat_pack(1, 1, &msg, MAV_TYPE_QUADROTOR, MAV_AUTOPILOT_ARDUPILOTMEGA, 
                                   MAV_MODE_GUIDED_ARMED, 0, MAV_STATE_ACTIVE);

        // 3. Serialize (Chuyển sang buffer thô)
        uint8_t plain_buf[MAVLINK_MAX_PACKET_LEN];
        uint16_t plain_len = mavlink_msg_to_send_buffer(plain_buf, &msg);

        // -----------------------------------------------------------
        // 4. MÃ HÓA (Encryption Step)
        // -----------------------------------------------------------
        
        // a. Tạo Nonce ngẫu nhiên (24 bytes)
        unsigned char nonce[crypto_secretbox_NONCEBYTES];
        randombytes_buf(nonce, sizeof nonce);

        // b. Chuẩn bị buffer chứa kết quả (Độ dài = Data + MAC)
        std::vector<unsigned char> ciphertext(crypto_secretbox_MACBYTES + plain_len);

        // c. Thực hiện mã hóa
        crypto_secretbox_easy(ciphertext.data(), plain_buf, plain_len, nonce, MY_KEY);

        // d. Đóng gói cuối cùng: [NONCE] + [CIPHERTEXT]
        std::vector<unsigned char> final_packet;
        final_packet.insert(final_packet.end(), nonce, nonce + sizeof nonce);
        final_packet.insert(final_packet.end(), ciphertext.begin(), ciphertext.end());

        // 5. Gửi gói tin ĐÃ MÃ HÓA
        sendto(sockfd, final_packet.data(), final_packet.size(), 0, 
               (struct sockaddr *)&dest_addr, sizeof(dest_addr));

        std::cout << "[Sender] Da gui goi tin ma hoa (" << final_packet.size() << " bytes)" << std::endl;

        sleep(1); 
    }

    close(sockfd);
    return 0;
}