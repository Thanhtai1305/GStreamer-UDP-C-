cmake_minimum_required(VERSION 3.10)
project(GStreamer_P2P_Call LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(case_1_video)
add_subdirectory(case_2_video_audio)
add_subdirectory(case_3_stream_webcam)
add_subdirectory(case_4_video_call)
add_subdirectory(case_5_stream_record)
# Nếu có thư mục cho case1 hoặc case2, thêm add_subdirectory ở đây, ví dụ:
# add_subdirectory(case1)

# Tạo thực thi main_exec (hỗ trợ tất cả case)
add_executable(main_exec main.cpp)
target_link_libraries(main_exec PRIVATE case1_lib case2_lib case3_lib video_call_lib record_lib)
# Nếu có case1_lib, thêm PRIVATE case1_lib