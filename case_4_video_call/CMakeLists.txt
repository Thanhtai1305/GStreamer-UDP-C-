cmake_minimum_required(VERSION 3.10)
project(video_call_lib LANGUAGES CXX)

# Tìm PkgConfig và các thư viện GStreamer, GLib
find_package(PkgConfig REQUIRED)
pkg_check_modules(GST REQUIRED gstreamer-1.0)
pkg_check_modules(GLIB REQUIRED glib-2.0)

# Tạo thư viện tĩnh từ video_call.cpp
add_library(video_call_lib STATIC video_call.cpp video_call.h)

# Đặt thư mục include cho thư viện
target_include_directories(video_call_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${GST_INCLUDE_DIRS}
    ${GLIB_INCLUDE_DIRS}
)

# Liên kết thư viện với GStreamer và GLib
target_link_libraries(video_call_lib PUBLIC ${GST_LIBRARIES} ${GLIB_LIBRARIES})