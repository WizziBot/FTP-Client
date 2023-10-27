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

#include <DataConnection.h>

namespace FTP {

using namespace std;

// Active connection
DataConnection::DataConnection(uint32_t control_addr){
    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket == -1) {
        perror("Error creating socket");
        conn_status = CONN_FAILED;
    }
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = control_addr;
    int connected = 0;
    while (!connected){
        // Randomly generated number for port
        int rand_port = rand()%(DATA_PORT_MAX-DATA_PORT_MIN + 1) + DATA_PORT_MIN;
        
        server_addr.sin_port = htons(rand_port);

        if (bind(client_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
            perror("Error binding data connection port.");
        } else {
            connected = 1;
        }
    }

    if (listen(client_socket, 1) == -1) {
        perror("Error listening on data connection");
        close(client_socket);
        conn_status = CONN_FAILED;
    }
}

// Passive connection
DataConnection::DataConnection(string dst_address){
    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket == -1) {
        perror("Error creating socket");
        exit(1);
    }
    // Parse address
    // First 4 bytes are address and last 2 are port information
    // in the format (h1,h2,h3,h4,p1,p2)
    uint32_t dst_ip;
    uint16_t dst_port;
    for (string::iterator ci = dst_address.begin(); ci != dst_address.end(); ci++){
        char c = *ci;
        // Do parsing stuff
        // Check from ( to )
        // separate sections by comma
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(dst_port);
    server_addr.sin_addr.s_addr = htonl(dst_ip);
}

DataConnection::~DataConnection(){
    if (conn_status == CONN_SUCCESS){
        conn_status = CONN_TERM;
        close(client_socket);
    }
}

}