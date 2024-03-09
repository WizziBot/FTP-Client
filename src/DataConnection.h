/*  FTP Client
    Copyright (C) 2023  Matthew Rodriguez

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#pragma once

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>
#include <atomic>
#include <vector>
#include <fstream>
#include <experimental/filesystem>

#include <StatusConstants.h>

#define SOCKET_READ_TIMEOUT_USEC 100000

namespace FTP {

using namespace std;
namespace fs = std::experimental::filesystem;

class DataConnection {

public:

/*  Defines an FTP passive data connection
    @param dst_address FTP address string received from server
*/
DataConnection(string dst_address);
~DataConnection();

/*
    Read from buffer and write it to data connection socket
    @param buffer char vector with file contents
*/
int dsend_binary(const string path);
/*
    Read from buffer and write it to data connection socket
    @param buffer string with text file contents
*/
int dsend_ascii(const string path);

/*
    Reads from the data connection until EOF.
*/
vector<char> drecv_eof();

/*
    Reads from the data connection and updates atomic variable transfer_progress.
    @param f_dst file name on host system
    @param fsize file size in bytes
    @param binary_mode is binary connection
*/
int drecv_async(string f_dst, int fsize, bool binary_mode);

eConnStatus getStatus(){
    return conn_status;
}

// Transfer progress is an integer between 0 and 100;
std::atomic_int transfer_progress = {0};
private:

eConnStatus conn_status = CONN_NOT_INIT;
int client_socket;
struct sockaddr_in server_addr;
};

}