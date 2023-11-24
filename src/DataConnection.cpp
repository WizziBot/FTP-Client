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


// Passive connection
DataConnection::DataConnection(string dst_address){
    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket == -1) {
        perror("Error creating socket");
        exit(1);
    }
    // Parse address
    // First 4 bytes are address and last 2 are port information
    // in the format (h1,h2,h3,h4,p1,p2) where h1-h4, p1-p2 are string representations of bytes
    uint32_t dst_ip;
    uint16_t dst_port;
    // Points to 8-bit segments of the dst_ip 32-bit ip addr or 16-bit port
    uint8_t *dstptr = (uint8_t*)(&dst_ip);

    int is_reading = 0;
    // Keep track of how many bytes have been read
    int byte_number = 0;
    // Maximum byte size is '255' which is 3 chars long, last byte is for null termination
    char curr_byte_str[] = {0,0,0,0};
    char *cbptr = curr_byte_str;
    for (string::iterator ci = dst_address.begin(); ci != dst_address.end(); ci++){
        char c = *ci;

        if (is_reading){
            switch (c) {
            case ')':
                is_reading = 0;
                // Allow case fall through to process last byte
            case ',':
                *dstptr = (uint8_t)atoi(curr_byte_str);
                memset(curr_byte_str,0,sizeof(curr_byte_str));
                cbptr = curr_byte_str;
                if (byte_number == 3) dstptr = (uint8_t*)&dst_port;
                else dstptr++;
                byte_number++;
                break;
            default:
                *cbptr = c;
                cbptr++;
                break;
            }
        }

        if (c == '(' && is_reading == 0) is_reading = 1;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(dst_port);
    server_addr.sin_addr.s_addr = htonl(dst_ip);

    if(connect(client_socket,(struct sockaddr*)&server_addr, sizeof(server_addr)) == -1){
        conn_status = CONN_TERM;
        close(client_socket);
        return;
    }
    conn_status = CONN_SUCCESS;
}

DataConnection::~DataConnection(){
    if (conn_status == CONN_SUCCESS){
        conn_status = CONN_TERM;
        close(client_socket);
    }
}

}