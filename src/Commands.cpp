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
#include <algorithm>
#include <cctype>
#include <sstream>
#include <iostream>
#include <thread>

namespace FTP {

using namespace std;

string ControlConnection::getResponse() {
    // MSG_PEEK to not disturb the buffer, MSG_WAITALL to ensure response code is received
    if (recv(client_socket,msg_recv_buffer,4,MSG_PEEK | MSG_WAITALL) == -1){
        cerr << "Error in receiving response to command" << endl;
        return string("");
    } else {
        string response = processResponseCode();
        setLastResponse(response);
        return response;
    }
}

string ControlConnection::ftp_login(const string username,const string password){
    string cmd_string = "USER ";
    cmd_string += username;
    log(cmd_string);
    cmd_string += "\r\n";
    send(client_socket,cmd_string.c_str(),cmd_string.length(),0);
    
    string response = getResponse();
    log(response);

    if (response.size() == 0 || response.at(0) != D1_INTERMEDIATE){
        return response;
    }
    cmd_string = "PASS ";
    cmd_string += password;
    log(cmd_string);
    cmd_string += "\r\n";
    send(client_socket,cmd_string.c_str(),cmd_string.length(),0);
    return getResponse();
}

// Print working directory
string ControlConnection::pwd(){
    char cmd_string[] = "PWD\r\n";
    log(string("PWD"));
    send(client_socket,cmd_string,sizeof(cmd_string)-1,0);
    return getResponse();
}

// Change working directory
string ControlConnection::cwd(const string new_dir){
    string cmd_string = "CWD ";
    cmd_string += new_dir;
    log(cmd_string);
    cmd_string += "\r\n";
    send(client_socket,cmd_string.c_str(),cmd_string.length(),0);
    return getResponse();
}

/*
    Requests the server to end the connection.
*/
string ControlConnection::quit(){
    char cmd_string[] = "QUIT\r\n";
    log(string("QUIT"));
    send(client_socket,cmd_string,sizeof(cmd_string)-1,0);
    return getResponse();
}

string ControlConnection::help(){
    static string help_cmds = 
    " login username [password] - Login with plaintext username and password of server.\n\n"
    " pwd - Print working directory.\n\n"
    " cd - Change workig directory.\n\n"
    " ls - Print directory contents.\n\n"
    " rm file_name - Delete file on remote server.\n\n"
    " mkdir dir_name - Create directory on remote server.\n\n"
    " rmdir dir_name - Remove directory on remote server.\n\n"
    " status - Print connection information.\n\n"
    " system - Prints ftp server system information.\n\n"
    " quit - Terminate connection and exit.\n\n"
    " help - bring up this menu.\n\n"
    " put source [dst_name] - upload source file to server, optnionally changing the name.\n\n"
    " get source [local_name] - download file from server, optnionally changing the name.\n\n"
    " type representation_type - ASCII or BINARY representation type selection for transfers.\n";
    return help_cmds;
}

string ControlConnection::type(string t_type){
    string cmd_string = "TYPE ";
    eDataType newType;
    // Convert input to lowercase by performing std::tolower() on every character in t_type using std::transform()
    transform(t_type.begin(), t_type.end(), t_type.begin(), [](unsigned char c){ return tolower(c); });
    // Only accept ascii and binary types
    if (t_type == "ascii"){
        cmd_string += "A\r\n";
        newType = DATA_ASCII;
    } else if (t_type == "binary"){
        cmd_string += "I\r\n";
        newType = DATA_BINARY;
    } else {
        return string("Invalid or Unsuported Type");
    }
    log(cmd_string);
    send(client_socket,cmd_string.c_str(),cmd_string.length(),0);
    string response = getResponse();
    if (response.at(0) == D1_COMPLETION){
        data_type = newType;
    }
    return response;
}

string ControlConnection::mode(const string t_mode){
    return string("Only stream mode is supported.");
}

string ControlConnection::syst(){
    char cmd_string[] = "SYST\r\n";
    log(string("SYST"));
    send(client_socket,cmd_string,sizeof(cmd_string)-1,0);
    return getResponse();
}

string ControlConnection::stat(){
    char cmd_string[] = "STAT\r\n";
    log(string("STAT"));
    send(client_socket,cmd_string,sizeof(cmd_string)-1,0);
    return getResponse();
}

string ControlConnection::dele(string f_name){
    string cmd_string = "DELE ";
    cmd_string += f_name;
    log(cmd_string);
    cmd_string += "\r\n";
    send(client_socket,cmd_string.c_str(),cmd_string.length(),0);
    return getResponse();
}

string ControlConnection::mkd(const string d_name){
    string cmd_string = "MKD ";
    cmd_string += d_name;
    log(cmd_string);
    cmd_string += "\r\n";
    send(client_socket,cmd_string.c_str(),cmd_string.length(),0);
    return getResponse();
}

string ControlConnection::rmd(const string d_name){
    string cmd_string = "RMD ";
    cmd_string += d_name;
    log(cmd_string);
    cmd_string += "\r\n";
    send(client_socket,cmd_string.c_str(),cmd_string.length(),0);
    return getResponse();
}

/*
    The below methods make use of the data connection.
*/

string ControlConnection::list(){
    // Similar method to retr()
    if (data_mode != DATA_PASSIVE) return string("");

    string cmd_string = "PASV\r\n";
    log(string("PASV"));
    send(client_socket,cmd_string.c_str(),cmd_string.length(),0);
    string response = getResponse();
    if (response.size() == 0 || response.at(0) != D1_COMPLETION){
        return response;
    }
    log(response);

    data_connection = new DataConnection(response);
    if (data_connection->getStatus() != CONN_SUCCESS) {
        return string("Data connection failed");
    }

    cmd_string = "LIST\r\n";
    log(string("LIST"));
    send(client_socket,cmd_string.c_str(),cmd_string.length(),0);
    response = getResponse();

    if (response.size() == 0 || response.at(0) != D1_PRELIMINARY){
        delete data_connection;
        return response;
    }
    log(response);
    
    vector<char> data_to_write = data_connection->drecv_eof();
    delete data_connection;

    string stdout_data(data_to_write.begin(),data_to_write.end());
    log(getResponse());

    return stdout_data;

}

int ControlConnection::stor(const string f_name,const string f_dst){
    // Only passive mode 
    if (data_mode != DATA_PASSIVE) return -1;

    // Activate passive mode on server
    string cmd_string = "PASV\r\n";
    log(string("PASV"));
    send(client_socket,cmd_string.c_str(),cmd_string.length(),0);
    string response = getResponse();
    if (response.size() == 0 || response.at(0) != D1_COMPLETION){
        return -2;
    }
    log(response);
    // Initiate data connection
    data_connection = new DataConnection(response);
    if (data_connection->getStatus() != CONN_SUCCESS) {
        setLastResponse("Data connection failed");
        return -2;
    }
    // Send store request
    cmd_string = "STOR ";
    cmd_string += f_dst;
    log(cmd_string);
    cmd_string += "\r\n";
    send(client_socket,cmd_string.c_str(),cmd_string.length(),0);
    response = getResponse();

    // Receive OK response from server
    if (response.size() == 0 || response.at(0) != D1_PRELIMINARY){
        return -2;
    }
    log(response);

    // Get file contents and send data to server

    fs::path p = f_name;
    string path = fs::absolute(p);
    if (data_type == DATA_BINARY){
        
        auto dsend_process = [this](string path){
            int bytes = data_connection->dsend_binary(path);
            delete data_connection;
            if (bytes > 0) getResponse();
            else setLastResponse("Failed to send file");
            transfer_in_progress = false;
        };
        transfer_in_progress = true;
        thread dsend_thread(dsend_process,path);
        dsend_thread.detach();
    } else if (data_type == DATA_ASCII) {
        
        auto dsend_process = [this](string path){
            int bytes = data_connection->dsend_ascii(path);
            delete data_connection;
            if (bytes > 0) getResponse();
            else setLastResponse("Failed to send file");
            transfer_in_progress = false;
        };
        transfer_in_progress = true;
        thread dsend_thread(dsend_process,path);
        dsend_thread.detach();
    }

    return 0;
}

int ControlConnection::retr(string f_name,string f_dst){
    // Only passive mode 
    if (data_mode != DATA_PASSIVE) return -1;

    // Activate passive mode on server
    string cmd_string = "PASV\r\n";
    log(string("PASV"));
    send(client_socket,cmd_string.c_str(),cmd_string.length(),0);
    string response = getResponse();
    if (response.size() == 0 || response.at(0) != D1_COMPLETION){
        return -2;
    }
    // Initiate data connection
    data_connection = new DataConnection(response);
    if (data_connection->getStatus() != CONN_SUCCESS) {
        setLastResponse("Data connection failed");
        return -2;
    }

    // Send store request
    cmd_string = "RETR ";
    cmd_string += f_name;
    log(cmd_string);
    cmd_string += "\r\n";
    send(client_socket,cmd_string.c_str(),cmd_string.length(),0);
    response = getResponse();

    // Receive OK response from server
    if (response.size() == 0 || response.at(0) != D1_PRELIMINARY){
        delete data_connection;
        return -2;
    }
    log(response);

    // Get size of payload
    char size_str[32] = {0};
    int parameter_index = response.find('(') + 1;
    char c = 0;
    for (int i=0; i < (int)sizeof(size_str) && parameter_index < (int)response.size(); i++,parameter_index++){
        c = response.at(parameter_index);
        if (c == ')') break;
        size_str[i] = c;
    }
    int size_bytes = atoi(size_str);
    if (size_bytes <= 0){ // Check that a valid size is parsed
        delete data_connection;
        return -2;
    }

    auto drecv_process = [this](string f_dst,int fsize,bool binary_mode){
        int bytes = data_connection->drecv_async(f_dst,fsize,binary_mode);
        delete data_connection;
        if (bytes > 0) getResponse();
        else setLastResponse("Failed to receive file");
        transfer_in_progress = false;
    };
    transfer_in_progress = true;
    if (data_type == DATA_BINARY){
        thread dsend_thread(drecv_process,f_dst,size_bytes,true);
        dsend_thread.detach();
    } else {thread dsend_thread(drecv_process,f_dst,size_bytes,false);
        dsend_thread.detach();

    }

    return 0;
}

}