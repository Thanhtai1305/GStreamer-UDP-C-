cmake_minimum_required(VERSION 3.10)
project(MAVLink_Encrypted_UDP)

# Thiet lap chuan C++ (C++17)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDART_REQUIRED True)

# Tat canh bao "address-of-packed-member" cua MAVLink
add_compile_options(-Wno-address-of-packed-member)

# Tim goi thu vien Libsodium (Tuong duong voi -lsodium)
find_package(PkgConfig REQUIRED)
pkg_check_modules(SODIUM REQUIRED libsodium)

# Them duong dan include
# Dau . : thu muc hien tai de code tim thay "mavlink/common/..."
include_directories(.)
include_directories(${SODIUM_INCLUDE_DIRS})

# Cau hinh file executable cho sender
add_executable(sender sender.cpp)
# Lien ket thu vien sodium vao sender
target_link_libraries(sender PRIVATE ${SODIUM_LIBRARIES})


# Cau hinh file executable cho receiver
add_executable(receiver receiver.cpp)
#Lien ket thu vien sodium vao receiver
target_link_libraries(receiver PRIVATE ${SODIUM_LIBRARIES})