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

#include <ControlConnection.h>

// Checks for digit between 0 and 9 inclusive
#define isDigit(x) (x >= 0x30 && x <= 0x39)

namespace FTP {

using namespace std;

ControlConnection::ControlConnection(const string dst_ip,const int dst_port){
    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket == -1) {
        perror("Error creating socket");
        exit(1);
    }
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(dst_port);
    server_addr.sin_addr.s_addr = inet_addr(dst_ip.c_str());
}

ControlConnection::~ControlConnection(){
    if (conn_status == CONN_SUCCESS){
        conn_status = CONN_TERM;
        close(client_socket);
    }
}

string ControlConnection::fromTelnet(char* buff,int buff_len){
    char s_msg[1024] = {0};
    for (int i=0; i<buff_len; i++){
        if (buff[i] == '\r') {
            s_msg[i] = '\0';
            break;
        }
        s_msg[i] = buff[i];
    }
    
    return string(s_msg);
}

// Reads from the start of long response till the end of it by reading the stop_code twice
// (RFC 959) 4.2. FTP REPLIES
/*
    ... Thus the format for multi-line replies is that the first line
    will begin with the exact required reply code, followed
    immediately by a Hyphen, "-" (also known as Minus), followed by
    text.  The last line will begin with the same code, followed
    immediately by Space <SP>, optionally some text, and the Telnet
    end-of-line code.

    For example:
        123-First line
        Second line
            234 A line beginning with numbers
        123 The last line
*/
string ControlConnection::readDataUntilCode(char* stop_code){
    int bytes_received = 1;
    int line_start = 0;
    int line_end = 0;
    int code_count = 0;
    int isLineEnd = 0;
    while (bytes_received != 0){
        bytes_received = recv(client_socket,(char*)msg_recv_buffer+line_end,sizeof(char),0);
        if (bytes_received == -1){
            cerr << "Error in receiving control data byte" << endl;
        } else {
            if (isLineEnd && msg_recv_buffer[line_end] == '\n'){
                // Check for stop code within line
                if (msg_recv_buffer[line_start] == stop_code[0] 
                 && msg_recv_buffer[line_start+1] == stop_code[1]
                 && msg_recv_buffer[line_start+2] == stop_code[2]) {
                    code_count++;
                }
                if (code_count == 2){
                    msg_recv_buffer[line_end+1] = '\0';
                    break;
                }
                line_start = line_end + 1;
            }
            isLineEnd = 0;
            // Detect line end
            if (msg_recv_buffer[line_end] == '\r') isLineEnd = 1;
            line_end++;

        }
    }
    return string(msg_recv_buffer);
}

string ControlConnection::processResponseCode(){
    char* s_code = msg_recv_buffer;
    // Return if s_code does not contain a response code
    if (!isDigit(s_code[0]) || !isDigit(s_code[1]) || !isDigit(s_code[2])){
        perror("Bad response code\n");
        return "";
    }

    if (s_code[3] == '-'){
        return readDataUntilCode(s_code);
    } else {
        int r_code = stoi(s_code);
        switch (r_code) {
        case S_GOODBYE:
            conn_status = CONN_TERM;
            close(client_socket);
            return string("221 Goodbye\n");
        default:
            if (recv(client_socket,msg_recv_buffer,sizeof(msg_recv_buffer),0) == -1){
                perror("Error in receiving server response\n");
            } else {
                string r_message = fromTelnet(msg_recv_buffer,sizeof(msg_recv_buffer));
                return r_message;
            }
            break;
        }
    }
    return "";
}

string ControlConnection::processUserCommand(vector<string> command_args){

    // Switch case from base command, pass parameters if correct count
    if (command_args.at(0) == "pwd"){
        return pwd();
    } else if (command_args.at(0) == "cd"){
        if (command_args.size() == 2){
            return cwd(command_args.at(1));
        }
        return string("Insufficient arguments");
    } else if (command_args.at(0) == "ls"){
        return list();
    } else if (command_args.at(0) == "status"){
        return stat();
    } else if (command_args.at(0) == "quit"){
        return quit();
    } else if (command_args.at(0) == "help"){
        return help();
    } else if (command_args.at(0) == "type"){
        if (command_args.size() == 2){
            return type(command_args.at(1));
        }
        return string("Insufficient arguments");
    } else if (command_args.at(0) == "mode"){
        if (command_args.size() == 2){
            return mode(command_args.at(1));
        }
        return string("Insufficient arguments");
    } else if (command_args.at(0) == "get"){
        if (command_args.size() == 3){
            int status = retr(command_args.at(1),command_args.at(2),-1);
            if (status == -1) return string ("");
            else if (status == -2) return getLastResponse();
            while (transfer_in_progress) {}; // Atomic bool acts as a semaphore
            return getLastResponse();
        } else if (command_args.size() == 2){
            int status = retr(command_args.at(1),command_args.at(1),-1);
            if (status == -1) return string ("");
            else if (status == -2) return getLastResponse();
            while (transfer_in_progress) {};
            return getLastResponse();
        }
        return string("Insufficient arguments");
    } else if (command_args.at(0) == "put"){
        if (command_args.size() == 3){
            int status = stor(command_args.at(1),command_args.at(2));
            if (status == -1) return string("");
            else if (status == -2) return getLastResponse();
            while (transfer_in_progress) {};
            return getLastResponse();
        } else if (command_args.size() == 2){
            int status = stor(command_args.at(1),command_args.at(1));
            if (status == -1) return string("");
            else if (status == -2) return getLastResponse();
            while (transfer_in_progress) {};
            return getLastResponse();
        }
        return string("Insufficient arguments");
    } else if (command_args.at(0) == "system"){
        return syst();
    } else if (command_args.at(0) == "rm"){
        if (command_args.size() == 2){
            return dele(command_args.at(1));
        }
        return string("Insufficient arguments");
    } else if (command_args.at(0) == "mkdir"){
        if (command_args.size() == 2){
            return mkd(command_args.at(1));
        }
        return string("Insufficient arguments");
    } else if (command_args.at(0) == "rmdir"){
        if (command_args.size() == 2){
            return rmd(command_args.at(1));
        }
        return string("Insufficient arguments");
    } else if (command_args.at(0) == "login") {
        if (command_args.size() == 3) {
            return ftp_login(command_args.at(1),command_args.at(2));
        } else if (command_args.size() == 2){
            return ftp_login(command_args.at(1),string());
        }
        return string("Insufficient arguments");
    }
    return string("Unknown Command");

}

int ControlConnection::initConnection(){
    if (connect(client_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1){
        return -1;
    }
    // Get Hello message from server
    if (recv(client_socket,msg_recv_buffer,sizeof(msg_recv_buffer),0) == -1){
        perror("Error in receiving server hello");
        close(client_socket);
        return -1;
    }
    // Check status code
    string r_message = fromTelnet(msg_recv_buffer,sizeof(msg_recv_buffer));
    log(r_message);
    if (r_message.at(0) != '2'){
        perror("Error in receiving server hello");
        conn_status = CONN_FAILED;
        close(client_socket);
        return -1;
    }
    conn_status = CONN_SUCCESS;
    return 0;
}

}