cmake_minimum_required(VERSION 3.10)
project(MAVLink_Encrypted_UDP)

# 1. Thiet lap chuan C++ (C++17)
set(CMAKE_CXX_STANDARD 17)

set(CMAKE_CXX_STANDARD_REQUIRED True)

# 2. Tat canh bao "address-of-packed-member"
add_compile_options(-Wno-address-of-packed-member)

# 3. Tim goi thu vien Libsodium
find_package(PkgConfig REQUIRED)
pkg_check_modules(SODIUM REQUIRED libsodium)

# 4. Tim goi thu vien Threads/pthread
find_package(Threads REQUIRED)

# 5. Include directories
include_directories(.)
include_directories(${SODIUM_INCLUDE_DIRS})



# Cau hinh cho sender
add_executable(sender sender.cpp)
target_link_libraries(sender PRIVATE ${SODIUM_LIBRARIES} Threads::Threads)



# Cau hinh cho receiver
add_executable(receiver receiver.cpp)
target_link_libraries(receiver PRIVATE ${SODIUM_LIBRARIES} Threads::Threads)