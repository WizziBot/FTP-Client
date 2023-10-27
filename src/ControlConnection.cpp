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

#define S_READY 220
#define S_STATUS_INDICATOR 211
#define S_GOODBYE 221
#define S_PASSIVE_MODE 227

#define D1_PRELIMINARY '1'
#define D1_COMPLETION '2'
#define D1_INTERMEDIATE '3'
#define D1_TRANSIENT_NEGATIVE '4'
#define D1_FAILURE '5'

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

pair<int,string> ControlConnection::fromTelnet(char* buff,int buff_len){
    char s_code[4] = {0,0,0,0};
    char s_msg[1024] = {0};
    int code;
    int isTelnet = 0;
    for (int i=0; i<buff_len; i++){
        if (i<3) s_code[i] = buff[i];
        if (buff[i] == '\r') {
            s_msg[i] = '\0';
            isTelnet = 1;
            break;
        }
        s_msg[i] = buff[i];
    }

    if (isTelnet == 0) {return make_pair(-2,string(s_msg));}
    try {
        code = stoi(s_code);
    } catch(const std::invalid_argument& e) {
        code = -1;
    }
    
    return make_pair(code,string(s_msg));
}

string ControlConnection::toTelnet(string command){
    return command.append("\r\n");
}

// Reads from the start of long response till the end of it by reading the stop_code twice
// RFC 4.2:
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
void ControlConnection::readDataUntilCode(char* stop_code){
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
                if (msg_recv_buffer[line_start] == stop_code[0] && msg_recv_buffer[line_start+1] == stop_code[1] && msg_recv_buffer[line_start+2] == stop_code[2]) {
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
    
    cout << msg_recv_buffer;
}

// TODO: Implement function map
int ControlConnection::processResponseCode(){
    int isOpenToNewCommand = 1;
    char* s_code = msg_recv_buffer;
    // Return if s_code does not contain a response code
    if (!isDigit(s_code[0]) || !isDigit(s_code[1]) || !isDigit(s_code[2])){
        perror("Bad response code\n");
        return 1;
    }
    if (s_code[0] == D1_PRELIMINARY){
        isOpenToNewCommand = 1;
    }
    // Convert code to integer for easy mapping
    if (s_code[3] == '-'){
        readDataUntilCode(s_code);
    } else {
        int r_code = stoi(s_code);
        cout << "CODE: " << r_code << endl;
        switch (r_code) {
        case S_GOODBYE:
            cout << "221 Goodbye" << endl;
            conn_status = CONN_TERM;
            close(client_socket);
            break;
        case S_PASSIVE_MODE:
            if (recv(client_socket,msg_recv_buffer,sizeof(msg_recv_buffer),0) == -1){
                perror("Error in receiving passive mode information\n");
            } else {
                pair<int,string> r_message = fromTelnet(msg_recv_buffer,sizeof(msg_recv_buffer));
                data_connection = new DataConnection(r_message.second);
            }
            break;
        default:
            cout << "[Unknown response code]: " << r_code << endl;
            if (recv(client_socket,msg_recv_buffer,sizeof(msg_recv_buffer),0) == -1){
                perror("Error in receiving server hello\n");
            } else {
                pair<int,string> r_message = fromTelnet(msg_recv_buffer,sizeof(msg_recv_buffer));
                cout << r_message.second << endl;
            }
            break;
        }
    }
    return isOpenToNewCommand;
}



void ControlConnection::interactive(){
    int openToNewCommands = 1;
    while (conn_status == CONN_SUCCESS){
        string command;
        if (openToNewCommands == 1){
            cout << ">";
            getline(cin, command);
            string send_command = toTelnet(command);
            send(client_socket,send_command.c_str(),send_command.length(),0);
        }
        // MSG_PEEK to not disturb the buffer, MSG_WAITALL to ensure response code is received
        if (recv(client_socket,msg_recv_buffer,4,MSG_PEEK | MSG_WAITALL) == -1){
            cerr << "Error in receiving response to command:" << endl;
            cerr << command << endl;
        } else {
            openToNewCommands = processResponseCode();
        }
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
    pair<int,string> r_message = fromTelnet(msg_recv_buffer,sizeof(msg_recv_buffer));
    cout << r_message.second << endl;
    if (r_message.first != S_READY){
        perror("Error in receiving server hello");
        close(client_socket);
        return -1;
    }
    conn_status = CONN_SUCCESS;
    return 0;
}


}