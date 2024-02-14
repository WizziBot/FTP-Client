cmake_minimum_required(VERSION 3.16)
project(ftp-client)

set(SOURCE_FILES ControlConnection.cpp
                 DataConnection.cpp
                 Commands.cpp)
add_library(ftpcore ${SOURCE_FILES} )
add_executable(ftp ftp.cpp)
target_link_libraries(ftp PRIVATE ftpcore stdc++fs)
install(TARGETS ftp DESTINATION ${FTP_INSTALL_BIN_DIR})
