#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#include <ControlConnection.h>

using namespace FTP;

int main() {
    ControlConnection Conn1("127.0.0.1");
    if (Conn1.initConnection() == -1){
        perror("Unable to start connection");
        return -1;
    }

    Conn1.interactive();

    return 0;
}