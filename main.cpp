#include <iostream>
#include <string>
#include "case1.h"
#include "case2.h"
#include "case3.h"
#include "case_4_video_call/video_call.h"
#include "case_5_stream_record/record.h"

int main(int argc, char *argv[]) {
    if (argc < 3) {
        std::cout << "Usage: ./main_exec [case1|case2|case3|case4|case5] [A|B|sender|receiver] [ip]\n";
        return 0;
    }

    std::string mode = argv[1];
    std::string role = argv[2];
    const char *remote_ip = (argc > 3) ? argv[3] : "192.168.15.53";

    if (mode == "case1") {
        std::cout << "Starting case1 with role: " << role << std::endl;
        if (role == "sender") {
            start_sender();
            }
        
        else if (role == "receiver") {
            start_receiver();
        }
        else {
            std::cout << "Invalid role for case1. Use 'sender' or 'receiver'.\n";
        } 
    }
    else if (mode == "case2") {
        std::cout << "Starting case2 with role: " << role << std::endl;
        if (role == "sender") {
            start_sender_case2();
        }
        else if (role == "receiver") {
            start_receiver_case2();
            
        }
        else {
            std::cout << "Invalid role for case2. Use 'sender' or 'receiver'.\n";
        }
    }
    else if (mode == "case3") {
        std::cout << "Starting case3 with role: " << role << std::endl;
        if (role == "sender") {
            start_sender_case3();
        }
        else if (role == "receiver") {
            start_receiver_case3();
            
        }
        else {
            std::cout << "Invalid role for case3. Use 'sender' or 'receiver'.\n";
        }
    }
    else if (mode == "case4") {
        std::cout << "Starting case4 with remote IP: " << remote_ip << ", Config: " << role << std::endl;
        int result = run_video_call_pipelines(remote_ip, role.c_str());
        if (result != 0) {
            std::cerr << "Failed to run video call pipelines for case4" << std::endl;
            return result;
        }
    }
    else if (mode == "case5") {
        std::cout << "Starting case5 with role: " << role << ", IP: " << remote_ip << std::endl;
        if (role == "sender") {
            int result = start_sender_case5(remote_ip);
            if (result != 0) {
                std::cerr << "Failed to run sender pipeline for case5" << std::endl;
                return result;
            }
        }
        else if (role == "receiver") {
            int result = start_receiver_case5();
            if (result != 0) {
                std::cerr << "Failed to run receiver pipeline for case5" << std::endl;
                return result;
            }
        }
        else {
            std::cout << "Invalid role for case5. Use 'sender' or 'receiver'.\n";
        }
    }
    else {
        std::cout << "Invalid mode.\n";
    }

    return 0;
}