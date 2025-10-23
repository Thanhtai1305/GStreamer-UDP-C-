cmake_minimum_required(VERSION 3.10)
project(RTSP_Streaming_Camera)

find_package(PkgConfig REQUIRED)

# Check for GStreamer and GStreamer RTSP Server
pkg_check_modules(GST REQUIRED gstreamer-1.0 gstreamer-rtsp-server-1.0)

# Include GStreamer directories and definitions
include_directories(${GST_INCLUDE_DIRS})
link_directories(${GST_LIBRARY_DIRS})
add_definitions(${GST_CFLAGS_OTHER})


# Server
# Create the executable for the RTSP server
add_executable(rtsp_server rtsp_server.cpp)
target_link_libraries(rtsp_server PRIVATE ${GST_LIBRARIES})


# Client
# Create the executable for the RTSP client
add_executable(rtsp_client rtsp_client.cpp)
target_link_libraries(rtsp_client PRIVATE ${GST_LIBRARIES})
