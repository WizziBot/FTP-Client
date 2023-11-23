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

ControlConnection::ControlConnection(const char* dst_ip){
    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket == -1) {
        perror("Error creating socket");
        exit(1);
    }
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(CONTROL_PORT);
    server_addr.sin_addr.s_addr = inet_addr(dst_ip);
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

string ControlConnection::toTelnet(string command){
    return command.append("\r\n");
}

// Reads from the start of long response till the end of it by reading the stop_code twice
// RFC 959 section 4.2:
/*
    Thus the format for multi-line replies is that the first line
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
                isLineEnd = 0;
                line_start = line_end + 1;
            } else isLineEnd = 0;
            // Detect line end
            if (msg_recv_buffer[line_end] == '\r') isLineEnd = 1;
            line_end++;

        }
    }
    return string(msg_recv_buffer);
}

// TODO: Implement function map
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
        cout << "CODE: " << r_code << endl;
        switch (r_code) {
        case S_GOODBYE:
            conn_status = CONN_TERM;
            close(client_socket);
            return string("221 Goodbye\n");
        case S_PASSIVE_MODE:
            if (recv(client_socket,msg_recv_buffer,sizeof(msg_recv_buffer),0) == -1){
                perror("Error in receiving passive mode information\n");
            } else {
                string r_message = fromTelnet(msg_recv_buffer,sizeof(msg_recv_buffer));
                data_connection = new DataConnection(r_message);
                return r_message;
            }
            break;
        default:
            if (recv(client_socket,msg_recv_buffer,sizeof(msg_recv_buffer),0) == -1){
                perror("Error in receiving server hello\n");
            } else {
                string r_message = fromTelnet(msg_recv_buffer,sizeof(msg_recv_buffer));
                return r_message;
            }
            break;
        }
    }
    return "";
}

string ControlConnection::processUserCommand(string command){
    // TODO
    // Parse arguments from spaces
    vector<string> cmd_split;
    string temp;
    stringstream cmd_stream(command);

    while(getline(cmd_stream,temp,' ')){
        cmd_split.push_back(temp);
    }

    // Switch case from base command, pass parameters if correct count
    if (cmd_split.at(0) == "pwd"){
        return pwd();
    } else if (cmd_split.at(0) == "cd"){
        if (cmd_split.size() == 2){
            return cwd(cmd_split.at(1));
        }
    } else if (cmd_split.at(0) == "ls"){
        return list();
    } else if (cmd_split.at(0) == "quit"){
        return quit();
    } else if (cmd_split.at(0) == "help"){
        return help();
    } else if (cmd_split.at(0) == "type"){
        if (cmd_split.size() == 2){
            return type(cmd_split.at(1));
        }
    } else if (cmd_split.at(0) == "mode"){
        if (cmd_split.size() == 2){
            return type(cmd_split.at(1));
        }
    } else if (cmd_split.at(0) == "get"){
        if (cmd_split.size() == 3){
            return retr(cmd_split.at(1),cmd_split.at(2));
        }
    } else if (cmd_split.at(0) == "put"){
        if (cmd_split.size() == 3){
            return stor(cmd_split.at(1),cmd_split.at(2));
        }
    } else if (cmd_split.at(0) == "system"){
        return syst();
    } else if (cmd_split.at(0) == "delete"){
        if (cmd_split.size() == 2){
            return type(cmd_split.at(1));
        }
    } else if (cmd_split.at(0) == "connect") {
        if (cmd_split.size() == 3) {
            return ftp_connect(cmd_split.at(1),cmd_split.at(2));
        }
        return string("Requires 2 arguments.");
    }
    return string("Unknown Command");

}

void ControlConnection::interactive(){
    while (conn_status == CONN_SUCCESS){
        string command;
        cout << ">";
        getline(cin, command);
        string response = processUserCommand(command);
        if (response != "") cout << response << endl;
    }
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
    cout << r_message << endl;
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