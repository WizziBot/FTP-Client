#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080

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

    // Set up server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT); // Port number
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1"); // Server IP address

    // Connect to the server
    if (connect(client_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("Error connecting to server");
        close(client_socket);
        exit(1);
    }

    printf("Connected to server...\n");

    // Send data to the server
    printf("Enter a message to send to the server: ");
    fgets(message, sizeof(message), stdin);
    printf("LENGTH: %d\n",(int)strlen(message));
    send(client_socket, message, strlen(message)+1, 0);

    // Close the socket
    close(client_socket);

    return 0;
}
