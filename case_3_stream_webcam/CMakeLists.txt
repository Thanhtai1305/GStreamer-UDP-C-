cmake_minimum_required(VERSION 3.10)
project(case3_lib LANGUAGES CXX)

add_library(case3_lib STATIC sender-webcam.cpp receiver-webcam.cpp)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GST REQUIRED gstreamer-1.0)

target_include_directories(case3_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${GST_INCLUDE_DIRS}
)

target_link_libraries(case3_lib PUBLIC ${GST_LIBRARIES})
