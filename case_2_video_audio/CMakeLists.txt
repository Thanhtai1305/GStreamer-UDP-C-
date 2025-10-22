# Yeu cau phien ban CMake toi thieu
cmake_minimum_required(VERSION 3.10)

#Dinh nghia ten du an va ngon ngu su dung C++
project(gstreamer_video_audio CXX)

#Tim goi PkgConfig, mot cong cu can thiet de CMake co the tim cac thu vien he thong
find_package(PkgConfig REQUIRED)

#Su dung PkgConfig de tim thu vien GStreamer 1.0
#Neu tim tihay, cac bien nhu GST_INCLUDE_DIRS va GST_LIBRARIES se duoc tao ra
pkg_check_modules(GST REQUIRED gstreamer-1.0)

#Them cac thu muc chua file header (.h) cua GStreamer vao duong dan include
include_directories(${GST_INCLUDE_DIRS})

#Tao file thuc thi sender tu file ma nguon sender.cpp
add_executable(sender sender_simple.cpp)

#Lien ket thu vien GStreamer da tim thay voi file thuc thi
target_link_libraries(sender PRIVATE ${GST_LIBRARIES})

#Tai file thuc thi receiver tu file ma nguon receiver.cpp
add_executable(receiver receiver.cpp)

#Lien ket thu vien GStreamer da tim thay voi file thuc thi
target_link_libraries(receiver PRIVATE ${GST_LIBRARIES})

#In ra thong bao sau khi cau hinh xong
message(STATUS "CMake configured successfully. Run 'make' to build project")
