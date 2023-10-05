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

#define PORT 21

namespace FTP {

using namespace std;

enum eConnStatus {
    CONN_NOT_INIT,
    CONN_FAILED,
    CONN_SUCCESS,
    CONN_TERM
};

class ControlConnection {

public:

/*  Defines an FTP control connection
    @param dst_ip the server's IPv4 address
*/
ControlConnection(const char* dst_ip);
~ControlConnection();

/*  Attempts to connect to the FTP server
    @returns 0 on success, -1 on failure.
*/
int initConnection();

/*  Starts an interactive session in the terminal where the user can issue commands
    to the server manually and receive its responses.
*/
void interactive();

eConnStatus getConStatus(){return conn_status;}

private:

void readDataUntilCode(char* stop_code);

/*
    Process the response code in the msg_recv_buffer
*/
int processResponseCode();

/*  Parse a Telnet string which is terminated by \r\n
    @returns null terminated response string and response code as a pair.
*/
pair<int,string> fromTelnet(char* buff,int buff_len);

/*  Put a string into telnet format ready to be sent.
    @param command null terminated command
    @returns a \r\n terminated control command string
*/
string toTelnet(string command);

eConnStatus conn_status = CONN_NOT_INIT;
int client_socket;
struct sockaddr_in server_addr;
char msg_recv_buffer[1024];
};
 
}