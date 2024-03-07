cmake_minimum_required(VERSION 3.16)

project(ftp-client-build)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -Wno-deprecated")

set(CMAKE_INSTALL_PREFIX ${PROJECT_SOURCE_DIR})

set(FTP_INSTALL_BIN_DIR ${PROJECT_SOURCE_DIR}/bin)

set(FTP_HEADERS_DIR ${PROJECT_SOURCE_DIR}/src)

include_directories(${FTP_HEADERS_DIR})

add_subdirectory(src)
add_subdirectory(Qt-GUI)


