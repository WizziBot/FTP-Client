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
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>
#include <vector>
#include <sstream>
#include <atomic>

#include <StatusConstants.h>
#include <DataConnection.h>

namespace FTP {

using namespace std;

class ControlConnection {

public:

/*  Defines an FTP control connection
    @param dst_ip the server's IPv4 address
*/
ControlConnection(const string dst_ip,const int dst_port = CONTROL_PORT);
~ControlConnection();

/*  Attempts to connect to the FTP server
    @returns 0 on success, -1 on failure.
*/
int initConnection();

/*
    Process the user command by separating it into stem and arguments
    @param command_args command arguments including command keyword
    @returns the response from the server
*/
string processUserCommand(vector<string> command_args);

/*
    Returns the status of the client-server connection
*/
eConnStatus getConStatus(){return conn_status;}

/*
    Gets the last response message received from the server
*/
string getLastResponse(){return lastResponse;}

/*
    Send USER and PASS commands to login.
*/
string ftp_login(string username,string password);
/*
    Print working directory
*/
string pwd();
/*
    Lists directory contents (Note: server response is not returned and may be accessed by getLastResponse())
    @returns A string with the directory listing.
*/
string list();
/*
    Terminate connection and exit program.
*/
string quit();
/*
    Print list of available commands
*/
string help();
/*
    Changes data type of transfer data (ASCII, IMAGE, EBCDIC)
    (EBCDIC not implemented)
    @param t_type data type
*/
string type(string t_type);
/*
    Changes mode of transfer (stream, block, compressed)
    (Not implemented)
    @param t_mode mode
*/
string mode(const string t_mode);
/*
    Copies remote file to local machine
    @param f_name remote file name
    @param f_dst local file name
*/
int retr(const string f_name,const string f_dst);
/*
    Copies local file to server
    @param f_name local file path
    @param f_dst remote file name
    @returns 0 on success, -1 if failure with no response, -2 if failure with response.
*/
int stor(const string f_name,const string f_dst);
/*
    Requests system information via control connection
*/
string syst();
/*
    Requests connection information via control connection
*/
string stat();
/*
    Deletes file on remote server
    @param f_name name of remote file
*/
string dele(const string f_name);
/*
    Creates new directory on remote server
*/
string mkd(const string d_name);
/*
    Removes directory on remote server
*/
string rmd(const string d_name);
/*
    Change working directory
*/
string cwd(const string new_dir);

void setLogger(void(*logger_ptr)(string)){
    log = logger_ptr;
}

int getTranferProgress(){
    if (data_connection && data_connection->getStatus() == CONN_SUCCESS) return data_connection->transfer_progress;
    else return 0;
}

eDataType getTransferType() {
    return data_type;
}

bool isTrasferInProgress(){
    return transfer_in_progress;
}

private:

/*
    Receive server response and return it to the caller as a string
*/
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

/*  Parse a Telnet string which is terminated by \r\n
    @returns null terminated response string;
*/
string fromTelnet(char* buff,int buff_len);

void setLastResponse(string newResponse){
    lastResponse = newResponse;
}

// Logging may be redirected.

static void defaultLog(string text) {
    cout << text << endl;
}
void (*log)(string) = defaultLog;

eDataMode data_mode = DATA_PASSIVE;
eDataType data_type = DATA_ASCII;
eConnStatus conn_status = CONN_NOT_INIT;
int client_socket;
struct sockaddr_in server_addr;
char msg_recv_buffer[1024];
string lastResponse;
atomic_bool transfer_in_progress = {false};

DataConnection* data_connection = NULL;
};
 
}