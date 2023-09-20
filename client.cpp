#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define CHECK(x,msg) if (x == -1) { \
    perror(msg);\
    close(client_socket);\
    exit(1);\
}
#define SEND_COMMAND(cmd) \
CHECK(send(client_socket,cmd,sizeof(cmd)-1,0),"Failed to send command")\
printf("Sent: %s\n",cmd);\
memset(message,0,sizeof(message));
#define RECV_RESPONSE() \
CHECK(recv(client_socket,message,sizeof(message),0),"Error receiving server message")\
printf("Recieved: %s\n",message);

using namespace std;


#define PORT 21

int main() {
    int client_socket;
    struct sockaddr_in server_addr;
    char message[1024];

    // Create a socket
    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket == -1) {
        perror("Error creating socket");
        exit(1);
    }
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    // Connect to the server
    CHECK(connect(client_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)),"Error connecting to server")
    printf("Connected to server...\n");

    //Receive server 220
    RECV_RESPONSE()
    
    //Send USER command
    SEND_COMMAND("USER ftpuser\r\n")

    //Receive server 331
    RECV_RESPONSE()

    //Send PASS command
    SEND_COMMAND("PASS ftpserv\r\n")

    //Receive server 230
    RECV_RESPONSE()

    //Send SYST command
    SEND_COMMAND("SYST\r\n")

    //Receive system type
    RECV_RESPONSE()

    //Send quit command
    SEND_COMMAND("QUIT\r\n")

    //Receive 221 Goodbye
    RECV_RESPONSE()

    //Connection is terminated by the server, close socket also
    close(client_socket);

    return 0;
}
