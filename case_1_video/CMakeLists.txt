cmake_minimum_required(VERSION 3.10)
project(case1_lib LANGUAGES CXX)

# Create the static library
add_library(case1_lib STATIC sender.cpp receiver.cpp)

# Find GStreamer package
find_package(PkgConfig REQUIRED)
pkg_check_modules(GST REQUIRED gstreamer-1.0)

# Include directories
target_include_directories(case1_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${GST_INCLUDE_DIRS}
)

# Link GStreamer libraries
target_link_libraries(case1_lib PUBLIC ${GST_LIBRARIES})





