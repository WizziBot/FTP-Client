cmake_minimum_required(VERSION 3.16)
project(ftp-client)

set(SOURCE_FILES ftp.cpp
                 ControlConnection.cpp
                 DataConnection.cpp
                 Commands.cpp)

add_executable(ftp ${SOURCE_FILES})
target_link_libraries(ftp stdc++fs)
install(TARGETS ftp DESTINATION ${DIVISIBLE_INSTALL_BIN_DIR})
