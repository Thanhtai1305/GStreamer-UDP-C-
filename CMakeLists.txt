cmake_minimum_required(VERSION 3.10)
project(rtsp_stream_case6)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GST REQUIRED gstreamer-1.0 gstreamer-rtsp-server-1.0)

include_directories(${GST_INCLUDE_DIRS})
link_directories(${GST_LIBRARY_DIRS})
add_definitions(${GST_CFLAGS_OTHER})

# Gộp thư mục case_6_rtsp_stream
add_subdirectory(case_6_rtsp_stream)

# Tạo executable
add_executable(main main.cpp)

# Link cả thư viện con và GStreamer
target_link_libraries(main
    PRIVATE
    case_6_rtsp_stream
    ${GST_LIBRARIES}
)
