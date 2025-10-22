cmake_minimum_required(VERSION 3.10)
project(case2_lib LANGUAGES CXX)

# Create the static library
add_library(case2_lib STATIC sender_simple.cpp receiver.cpp)

# Find GStreamer package
find_package(PkgConfig REQUIRED)
pkg_check_modules(GST REQUIRED gstreamer-1.0)

# Include directories
target_include_directories(case2_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${GST_INCLUDE_DIRS}
)

# Link GStreamer libraries
target_link_libraries(case2_lib PUBLIC ${GST_LIBRARIES})
