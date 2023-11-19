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

#define S_READY 220
#define S_STATUS_INDICATOR 211
#define S_GOODBYE 221
#define S_PASSIVE_MODE 227

#define D1_PRELIMINARY '1'
#define D1_COMPLETION '2'
#define D1_INTERMEDIATE '3'
#define D1_TRANSIENT_NEGATIVE '4'
#define D1_FAILURE '5'

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>
#include <vector>
#include <sstream>

#include <StatusConstants.h>
#include <DataConnection.h>

namespace FTP {

using namespace std;

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

string ftp_connect(string username,string password);
string ftp_connect(string username);

string pwd();
string list();
string quit();
string help();
string type(string t_type);
string mode(string t_mode);
string retr(string f_name,string f_dst);
string stor(string f_name,string f_dst);
string syst();
string dele(string f_name);
string cwd(string new_dir);
string getResponse();

/*
    Processes multiple message responses
    @returns the whole response as a string 
*/
string readDataUntilCode(char* stop_code);

/*
    Process the response code in the msg_recv_buffer
*/
string processResponseCode();

/*
    Process the user command by separating it into stem and arguments
    @returns the response from the server to be printer/processed otherwise
*/
string processUserCommand(string command);

/*  Parse a Telnet string which is terminated by \r\n
    @returns null terminated response string;
*/
string fromTelnet(char* buff,int buff_len);

/*  Put a string into telnet format ready to be sent.
    @param command null terminated command
    @returns a \r\n terminated control command string
*/
string toTelnet(string command);

eConnStatus conn_status = CONN_NOT_INIT;
int client_socket;
struct sockaddr_in server_addr;
char msg_recv_buffer[1024];

DataConnection* data_connection;
};
 
}