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
#include <vector>

#include <StatusConstants.h>

namespace FTP {

using namespace std;

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
int dsend(const vector<char> &buffer);
/*
    Read from buffer and write it to data connection socket
    @param buffer string with text file contents
*/
int dsend(const string &buffer);
/*
    Reads from ongoing data connection and returns the received bytes.
    @param size number of bytes to read from connection
*/
vector<char> drecv(int size);

eConnStatus getStatus(){
    return conn_status;
}

private:

eConnStatus conn_status = CONN_NOT_INIT;
int client_socket;
struct sockaddr_in server_addr;
};

}