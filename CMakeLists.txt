cmake_minimum_required(VERSION 3.14)
project(MockCameraDrone LANGUAGES CXX)

# C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Tìm GStreamer + RTSP Server
find_package(PkgConfig REQUIRED)
pkg_check_modules(GST REQUIRED 
    gstreamer-1.0>=1.14
    gstreamer-rtsp-server-1.0>=1.14
)
find_package(Threads REQUIRED)

# TÌM MAVLINK HEADER TỪ THƯ MỤC c_library_v2 NGAY CÙNG CẤP
set(MAVLINK_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/mavlink")
if(NOT EXISTS "${MAVLINK_INCLUDE_DIR}/common/mavlink.h")
    message(FATAL_ERROR "
        Không tìm thấy file mavlink.h trong thư mục mavlink!
        Kiểm tra lại cấu trúc:
        mavlink/common/mavlink.h
    ")
endif()

# Tạo executable
add_executable(mock_camera_drone mock_camera_drone.cpp)

# Include directories
target_include_directories(mock_camera_drone PRIVATE
    ${GST_INCLUDE_DIRS}
    ${MAVLINK_INCLUDE_DIR}              # ← Đây là đường dẫn chính xác
    ${MAVLINK_INCLUDE_DIR}/common       # ← Cho #include "mavlink/common/mavlink.h"
)

# Link libraries (chỉ cần GStreamer + pthread)
target_link_libraries(mock_camera_drone PRIVATE
    Threads::Threads
    ${GST_LIBRARIES}
)

# Compiler flags
target_compile_options(mock_camera_drone PRIVATE
    ${GST_CFLAGS_OTHER}
    -Wall -Wextra -O2
)

# Thông báo thành công
message(STATUS "========================================")
message(STATUS "  MOCK CAMERA DRONE BUILD SUCCESS!")
message(STATUS "  MAVLink headers: ${MAVLINK_INCLUDE_DIR}")
message(STATUS "  RTSP Stream    : rtsp://127.0.0.1:8554/webcam")
message(STATUS "  Run            : ./mock_camera_drone")
message(STATUS "========================================")