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
#include <math.h>
#include <string.h>

#define TRANSMISSION_UNIT 1024
#define FILE_CHUNK_SIZE 1048576

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
    // in the format (h1,h2,h3,h4,p1,p2) where h1-h4, p1-p2 are numeric string representations of bytes from highest to lowest order.
    uint16_t dst_port = 0;
    uint32_t dst_ip = 0;

    int is_reading = 0;
    // Keep track of how many bytes have been read
    int byte_number = 0;
    uint8_t next_byte = 0;
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
                next_byte = (uint8_t)atoi(curr_byte_str); // Assign currently stored string as a number to the location of dstptr
                memset(curr_byte_str,0,sizeof(curr_byte_str)); // Clear char array
                cbptr = curr_byte_str; // update cbptr to the start of the char array
                if (byte_number < 4) { // byte numbers 0,1,2,3 are part of the IP whereas 4 and 5 are the port.
                    dst_ip |= next_byte << (3-byte_number)*8; //Left shift next byte into place
                } else {
                    dst_port |= next_byte << (5-byte_number)*8;
                }
                byte_number++;
                break;
            default:
                *cbptr = c; // By default append character to char array
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

int DataConnection::dsend_binary(const string path){

    // Setup file io stream in binary mode
    ifstream file(path,std::ios::binary | std::ios::ate);
    streampos fsize = file.tellg();
    file.seekg(0, std::ios::beg);

    // Allocate memory of FILE_CHUNK_SIZE bytes and then write into it using file.read(destination,size)
    // Note that char is always 1 byte in size in the C standard.
    char* buffer = (char*)malloc(FILE_CHUNK_SIZE);
    int total_sent = 0;
    int bytes_sent = 0;
    streampos lastpos = 0;

    // Read large files in FILE_CHUNK_SIZE sized chunks
    while (!file.eof() && total_sent < fsize) {
        int chunk_sent = 0;
        memset(buffer,0,FILE_CHUNK_SIZE); // Zero memory buffer used for file contents.
        streampos read_size = std::min((streampos)FILE_CHUNK_SIZE,(streampos)(fsize-file.tellg()));
        file.read(buffer, read_size); // Read only the necessary amount if its last chunk
        if(file.fail()) {
            cerr << "Filed to read file." << endl;
            free(buffer);
            return -1;
        } else if (file.bad()){
            cerr << "Bad file read." << endl;
            free(buffer);
            return -1;
        }

        int buffer_len = file.tellg() - lastpos;
        lastpos = file.tellg();

        // Iterate over the chunk and send data in further 1024 byte transmision units
        while (chunk_sent < buffer_len) {
            int send_size = min(TRANSMISSION_UNIT,buffer_len);
            bytes_sent = send(client_socket, buffer + chunk_sent, send_size, 0);
            if (bytes_sent == -1) {
                cerr << "Error sending bytes." <<endl;
                free(buffer);
                return -1;
            }
            chunk_sent += bytes_sent;
            total_sent += bytes_sent;
            transfer_progress = (total_sent * 100)/fsize;
        }
    }

    free(buffer); // Remember to free buffer

    return total_sent;
}

int DataConnection::dsend_ascii(const string path){

    // Setup file io stream
    ifstream file(path, std::ios::ate);
    streampos fsize = file.tellg();
    file.seekg(0, std::ios::beg);

    char* buffer = (char*)malloc(FILE_CHUNK_SIZE);
    int total_sent = 0;
    int bytes_sent = 0;
    streampos lastpos = 0;

    while (!file.eof() && total_sent < fsize){
        int chunk_sent = 0;
        memset(buffer,0,FILE_CHUNK_SIZE); // Zero memory buffer used for file contents.
        streampos read_size = std::min((streampos)FILE_CHUNK_SIZE,(streampos)(fsize-file.tellg()));
        file.read(buffer, read_size); // Read only the necessary amount if its last chunk
        if(file.fail()) {
            cerr << "Filed to read file." << endl;
            free(buffer);
            return -1;
        } else if (file.bad()){
            cerr << "Bad file read." << endl;
            free(buffer);
            return -1;
        }

        int buffer_len = file.tellg() - lastpos;
        lastpos = file.tellg();

        while (chunk_sent < buffer_len) {
            int send_size = min(TRANSMISSION_UNIT,buffer_len);
            bytes_sent = send(client_socket, buffer + chunk_sent, send_size, 0);
            if (bytes_sent == -1) {
                cerr << "Error sending bytes." <<endl;
                free(buffer);
                return -1;
            }
            chunk_sent += bytes_sent;
            total_sent += bytes_sent;
            transfer_progress = (total_sent * 100)/fsize;
        }
    }

    free(buffer);

    return total_sent;
}

vector<char> DataConnection::drecv_eof(){
    if (conn_status != CONN_SUCCESS) return vector<char>();
    vector<char> recv_buf; // Pre-allocate 1024 bytes
    recv_buf.reserve(1024);
    char temp;
    while (1) {
        int n_bytes = recv(client_socket,&temp,sizeof(char),0);
        switch (n_bytes) {
        case -1:
            cerr << "Failed to read from data connection." << endl;
            recv_buf.shrink_to_fit(); // Free unused memory
            return recv_buf;
        case 0:
            // EOF implied by closing of data connection.
            /* (RFC 959) 3.4.  TRANSMISSION MODES
                ... All data transfers must be completed with an end-of-file (EOF)
                which may be explicitly stated or implied by the closing of the
                data connection.  For files with record structure, all the
                end-of-record markers (EOR) are explicit, including the final one.
                For files transmitted in page structure a "last-page" page type is
                used.
            */
            recv_buf.shrink_to_fit();
            return recv_buf;
        default:
            recv_buf.push_back(temp);
        }
    }
    return recv_buf;
}

}