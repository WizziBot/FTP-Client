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

namespace FTP {

using namespace std;

string ControlConnection::getResponse() {
    // MSG_PEEK to not disturb the buffer, MSG_WAITALL to ensure response code is received
    if (recv(client_socket,msg_recv_buffer,4,MSG_PEEK | MSG_WAITALL) == -1){
        cerr << "Error in receiving response to command: USER" << endl;
        return "";
    } else {
        return processResponseCode();
    }
}

string ControlConnection::ftp_connect(string username,string password){
    string cmd_string = "USER ";
    cmd_string += username;
    cmd_string += "\r\n";
    send(client_socket,cmd_string.c_str(),cmd_string.length(),0);
    
    string response = getResponse();
    if (response.size() == 0 || response.at(0) != D1_INTERMEDIATE){
        return response;
    }
    cmd_string = "PASS ";
    cmd_string += password;
    cmd_string += "\r\n";
    send(client_socket,cmd_string.c_str(),cmd_string.length(),0);
    string response2 = getResponse();
    return response2;
}

string ControlConnection::pwd(){
    char cmd_string[] = "PWD\r\n";
    send(client_socket,cmd_string,sizeof(cmd_string)-1,0);
    return getResponse();
}

string ControlConnection::cwd(string new_dir){
    string cmd_string = "CWD ";
    cmd_string += new_dir;
    cmd_string += "\r\n";
    send(client_socket,cmd_string.c_str(),cmd_string.length(),0);
    return getResponse();
}

string ControlConnection::list(){
    char cmd_string[] = "LIST\r\n";
    send(client_socket,cmd_string,sizeof(cmd_string)-1,0);
    return getResponse();
}

string ControlConnection::quit(){
    char cmd_string[] = "QUIT\r\n";
    send(client_socket,cmd_string,sizeof(cmd_string)-1,0);
    return getResponse();
}

string ControlConnection::help(){
    char cmd_string[] = "HELP\r\n";
    send(client_socket,cmd_string,sizeof(cmd_string)-1,0);
    return getResponse();
}

string ControlConnection::type(string t_type){
    string cmd_string = "TYPE ";
    cmd_string += t_type;
    cmd_string += "\r\n";
    send(client_socket,cmd_string.c_str(),cmd_string.length(),0);
    return getResponse();
}

string ControlConnection::mode(string t_mode){
    string cmd_string = "MODE ";
    cmd_string += t_mode;
    cmd_string += "\r\n";
    send(client_socket,cmd_string.c_str(),cmd_string.length(),0);
    return getResponse();
}

string ControlConnection::syst(){
    char cmd_string[] = "SYST\r\n";
    send(client_socket,cmd_string,sizeof(cmd_string)-1,0);
    return getResponse();
}

string ControlConnection::dele(string f_name){
    string cmd_string = "DELE ";
    cmd_string += f_name;
    cmd_string += "\r\n";
    send(client_socket,cmd_string.c_str(),cmd_string.length(),0);
    return getResponse();
}

// Do work on these functions when we get onto Data Connection
string ControlConnection::retr(string f_name,string f_dst){
    string cmd_string = "RETR ";
    cmd_string += f_name;
    cmd_string += "\r\n";
    send(client_socket,cmd_string.c_str(),cmd_string.length(),0);
    
    string response = getResponse();
}

string ControlConnection::stor(string f_name,string f_dst){
    string cmd_string = "STOR ";
    cmd_string += f_dst;
    cmd_string += "\r\n";
    send(client_socket,cmd_string.c_str(),cmd_string.length(),0);
    return getResponse();
}

}