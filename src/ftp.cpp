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

#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#include <ControlConnection.h>

using namespace FTP;

int main(int argc, char** argv) {
    
    if (argc < 2 || argc > 3){
        std::cerr << "Usage: " + std::string(argv[0]) + " server_ipv4_address (port)" << std::endl;
        return -1;
    }
    ControlConnection* Conn1;
    string address = string(argv[1]);
    if (argc == 2){
        Conn1 = new ControlConnection(address);
    } else {
        int port = atoi(argv[2]);
        Conn1 = new ControlConnection(address,port);
    }
    if (Conn1->initConnection() == -1){
        perror("Unable to start connection");
        return -1;
    }

    while (Conn1->getConStatus() == CONN_SUCCESS){
        string command;
        cout << ">";
        getline(cin, command);
        vector<string> cmd_split;
        string temp;
        stringstream cmd_stream(command);

        while(getline(cmd_stream,temp,' ')){
            if (temp.at(0) == '\"') {
                string rest;
                getline(cmd_stream,rest);
                temp = temp + " " + rest;
                temp = temp.substr(1,temp.length()-2);
            }
            cmd_split.push_back(temp);
        }
        string response = Conn1->processUserCommand(cmd_split);
        if (response != "") cout << response << endl;
    }

    delete Conn1;

    return 0;
}