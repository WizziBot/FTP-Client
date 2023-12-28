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
#include <fstream>
#include <sstream>
#include <iostream>
#include <experimental/filesystem>

namespace fs = std::experimental::filesystem;

namespace FTP {

using namespace std;

string ControlConnection::getResponse() {
    // MSG_PEEK to not disturb the buffer, MSG_WAITALL to ensure response code is received
    if (recv(client_socket,msg_recv_buffer,4,MSG_PEEK | MSG_WAITALL) == -1){
        cerr << "Error in receiving response to command" << endl;
        return string("");
    } else {
        return processResponseCode();
    }
}

string ControlConnection::ftp_login(const string username,const string password){
    string cmd_string = "USER ";
    cmd_string += username;
    cmd_string += "\r\n";
    send(client_socket,cmd_string.c_str(),cmd_string.length(),0);
    
    string response = getResponse();

    if (response.size() != 0 && response.at(0) == D1_COMPLETION){
        return response;
    }

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

// Print working directory
string ControlConnection::pwd(){
    char cmd_string[] = "PWD\r\n";
    send(client_socket,cmd_string,sizeof(cmd_string)-1,0);
    return getResponse();
}

// Change working directory
string ControlConnection::cwd(const string new_dir){
    string cmd_string = "CWD ";
    cmd_string += new_dir;
    cmd_string += "\r\n";
    send(client_socket,cmd_string.c_str(),cmd_string.length(),0);
    return getResponse();
}

/*
    Requests the server to end the connection.
*/
string ControlConnection::quit(){
    char cmd_string[] = "QUIT\r\n";
    send(client_socket,cmd_string,sizeof(cmd_string)-1,0);
    return getResponse();
}

string ControlConnection::help(){
    static string help_cmds = 
    "> login [username] [password] - Login with plaintext username and password of server\n\n"
    "> pwd - Print working directory\n\n"
    "> cd - Change workig directory\n\n"
    "> ls - Print directory contents\n\n"
    "> quit - Terminate connection and exit\n\n"
    "> ";
    return help_cmds;
}

string ControlConnection::type(string t_type){
    string cmd_string = "TYPE ";
    // Convert input to lowercase by performing std::tolower() on every character in t_type using std::transform()
    transform(t_type.begin(), t_type.end(), t_type.begin(), [](unsigned char c){ return tolower(c); });
    // Only accept ascii and binary types
    if (t_type == "ascii"){
        cmd_string += "A\r\n";
    } else if (t_type == "binary"){
        cmd_string += "I\r\n";
    } else {
        return string("Invalid or Unsuported Type");
    }
    send(client_socket,cmd_string.c_str(),cmd_string.length(),0);
    string response = getResponse();
    if (response.at(0) == D1_COMPLETION){
        data_type = DATA_BINARY;
    }
    return response;
}

string ControlConnection::mode(const string t_mode){
    return string("Only stream mode is supported.");
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

/*
    The below methods make use of the data connection.
*/

string ControlConnection::list(){
    // Similar method to retr()
    if (data_mode != DATA_PASSIVE) return string("");

    string cmd_string = "PASV\r\n";
    send(client_socket,cmd_string.c_str(),cmd_string.length(),0);
    string response = getResponse();
    if (response.size() == 0 || response.at(0) != D1_COMPLETION){
        return response;
    }

    data_connection = new DataConnection(response);
    if (data_connection->getStatus() != CONN_SUCCESS) {
        return string("Data connection failed");
    }

    cmd_string = "LIST\r\n";
    send(client_socket,cmd_string.c_str(),cmd_string.length(),0);
    response = getResponse();

    if (response.size() == 0 || response.at(0) != D1_PRELIMINARY){
        delete data_connection;
        return response;
    }
    
    vector<char> data_to_write = data_connection->drecv_eof();
    delete data_connection;

    string stdout_data(data_to_write.begin(),data_to_write.end());
    
    cout << stdout_data << endl;

    return getResponse();

}

string ControlConnection::stor(const string f_name,const string f_dst){
    // Only passive mode 
    if (data_mode != DATA_PASSIVE) return string("");

    // Activate passive mode on server
    string cmd_string = "PASV\r\n";
    send(client_socket,cmd_string.c_str(),cmd_string.length(),0);
    string response = getResponse();
    if (response.size() == 0 || response.at(0) != D1_COMPLETION){
        return response;
    }
    // Initiate data connection
    data_connection = new DataConnection(response);
    if (data_connection->getStatus() != CONN_SUCCESS) {
        return string("Data connection failed");
    }
    // Send store request
    cmd_string = "STOR ";
    cmd_string += f_dst;
    cmd_string += "\r\n";
    send(client_socket,cmd_string.c_str(),cmd_string.length(),0);
    response = getResponse();

    // Receive OK response from server
    if (response.size() == 0 || response.at(0) != D1_PRELIMINARY){
        return response;
    }

    // Get file contents and send data to server

    fs::path p = f_name;
    string path = fs::absolute(p);

    if (data_type == DATA_BINARY){
        
        // Setup file io stream in binary mode
        ifstream file(path,std::ios::binary | std::ios::ate);
        streamsize fsize = file.tellg();
        file.seekg(0, std::ios::beg);

        // Allocate memory in a vector of 'char' type and then write into it using file.read(destination,size)
        // Note that char is always 1 byte in size in the C standard.
        // buffer.data() returns a pointer to the start of the allocated memory for buffer.
        vector<char> buffer(fsize);
        if (!file.read(buffer.data(), fsize)) {
            delete data_connection;
            return string("Error reading file ") + f_name;
        }

        data_connection->dsend(buffer);
        delete data_connection;
    } else if (data_type == DATA_ASCII) {
        // Setup file io stream
        ifstream file(path);
        string buffer;
        
        // Read file line by line and append to the buffer
        string line;
        while (getline(file,line)){
            buffer += line;
        }

        cout << "Buffer: " << buffer << endl;

        data_connection->dsend(buffer);
        delete data_connection;
    }

    return getResponse();
}

string ControlConnection::retr(string f_name,string f_dst){
    // Only passive mode 
    if (data_mode != DATA_PASSIVE) return string("");

    // Activate passive mode on server
    string cmd_string = "PASV\r\n";
    send(client_socket,cmd_string.c_str(),cmd_string.length(),0);
    string response = getResponse();
    if (response.size() == 0 || response.at(0) != D1_COMPLETION){
        return response;
    }
    // Initiate data connection
    data_connection = new DataConnection(response);
    if (data_connection->getStatus() != CONN_SUCCESS) {
        return string("Data connection failed");
    }

    // Send store request
    cmd_string = "RETR ";
    cmd_string += f_name;
    cmd_string += "\r\n";
    send(client_socket,cmd_string.c_str(),cmd_string.length(),0);
    response = getResponse();

    // Receive OK response from server
    if (response.size() == 0 || response.at(0) != D1_PRELIMINARY){
        delete data_connection;
        return response;
    }

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
        return response;
    }

    // Receive payload through data connection

    vector<char> data_to_write = data_connection->drecv(size_bytes);
    delete data_connection;
    
    cout << "Writing " << data_to_write.size() << " bytes to " << f_dst << endl;

    if (data_type == DATA_BINARY){
        // Setup file io stream in binary mode then write to it and finally close
        ofstream file(f_dst.c_str(),std::ios::binary);
        file.write(data_to_write.data(),data_to_write.size());
        file.close();
    } else if (data_type == DATA_ASCII) {
        ofstream file(f_dst.c_str());
        file.write(data_to_write.data(),data_to_write.size());
        file.close();
    }

    return getResponse();
}

}