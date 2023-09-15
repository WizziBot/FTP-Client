#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080

int main() {
    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len;
    char buffer[1024];

    // Create a socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        perror("Error creating socket");
        exit(1);
    }

    // Set up server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT); // Port number
    server_addr.sin_addr.s_addr = INADDR_ANY;

    // Bind the socket to the server address
    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("Error binding");
        close(server_socket);
        exit(1);
    }

    // Listen for incoming connections
    if (listen(server_socket, 5) == -1) {
        perror("Error listening");
        close(server_socket);
        exit(1);
    }

    printf("Server is listening on port 8080...\n");

    // Accept a client connection
    client_addr_len = sizeof(client_addr);
    client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_addr_len);
    if (client_socket == -1) {
        perror("Error accepting client connection");
        close(server_socket);
        exit(1);
    }

    printf("Client connected...\n");

    // Receive data from the client
    ssize_t bytes_received = recv(client_socket, buffer, sizeof(buffer), 0);
    if (bytes_received == -1) {
        perror("Error receiving data");
        close(client_socket);
        close(server_socket);
        exit(1);
    }

    printf("Received data from client: %s\n", buffer);
    printf("LENGTH: %d\n",(int)strlen(buffer));

    // Close sockets
    close(client_socket);
    close(server_socket);

    return 0;
}
