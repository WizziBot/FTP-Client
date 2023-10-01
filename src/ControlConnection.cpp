
#include <ControlConnection.h>

#define S_READY 220
#define S_STATUS_INDICATOR 211

namespace FTP {

using namespace std;

ControlConnection::ControlConnection(const char* dst_ip){
    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket == -1) {
        perror("Error creating socket");
        exit(1);
    }
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr(dst_ip);
}

ControlConnection::~ControlConnection(){
    printf("Destructor called");
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
    printf("--a--");
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

void ControlConnection::readDataUntilCode(int stop_code){
    int bytes_received = 1;
    int loc_pointer = 0;
    char s_stop[5];
    sprintf(s_stop,"%d ",stop_code);
    printf("needle: %s\n",s_stop);
    fflush(stdout);
    while (bytes_received != 0){
        bytes_received = recv(client_socket,(char*)msg_recv_buffer+loc_pointer,sizeof(msg_recv_buffer),0);
        if (bytes_received == -1){
            cerr << "Error in receiving control data segment" << endl;
        } else {
            if (strstr(msg_recv_buffer,s_stop)){
                break;
            }
        }
    }
    recv(client_socket,(char*)msg_recv_buffer+loc_pointer,sizeof(msg_recv_buffer),MSG_DONTWAIT);
    cout << msg_recv_buffer << endl;
}

// TODO: Implement function map
void ControlConnection::processResponseCode(char* s_code){
    int r_code;
    s_code[4] = '\0';
    try {
        r_code = stoi(s_code);
    } catch(const std::invalid_argument& e) {
        return;
    }
    cout << "CODE: " << r_code << endl;
    switch (r_code)
    {
    case S_STATUS_INDICATOR:
        readDataUntilCode(S_STATUS_INDICATOR);
        break;
    default:
        cout << "[Unknown response code]: " << r_code << endl;
        if (recv(client_socket,msg_recv_buffer,sizeof(msg_recv_buffer),0) == -1){
            perror("Error in receiving server hello");
            conn_status = CONN_TERM;
            close(client_socket);
        } else {
            pair<int,string> r_message = fromTelnet(msg_recv_buffer,sizeof(msg_recv_buffer));
            cout << r_message.second << endl;
        }
        break;
    }
}



void ControlConnection::interactive(){
    while (conn_status == CONN_SUCCESS){
        string command;
        cout << ">";
        getline(cin, command);
        string send_command = toTelnet(command);
        send(client_socket,send_command.c_str(),send_command.length(),0);
        if (recv(client_socket,msg_recv_buffer,3,MSG_PEEK) == -1){
            cerr << "Error in receiving response to command:" << endl;
            cerr << command << endl;
        } else {
            processResponseCode(msg_recv_buffer);
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