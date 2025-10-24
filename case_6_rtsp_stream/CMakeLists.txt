cmake_minimum_required(VERSION 3.10)


add_library(case_6_rtsp_stream
    rtsp_server.cpp
    rtsp_client.cpp
)

target_include_directories(case_6_rtsp_stream PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
