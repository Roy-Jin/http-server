#include "src/server.h"
#include <stdio.h>
#include <stdlib.h>

SOCKET create_server_socket(int port);
void run_server(SOCKET serverSocket, int port);

int main(int argc, char* argv[]) {
    printf("\x1b[32m\x1b[47m\x1b[1m Created by Roy-Jin. \x1b[0m\n");

    int port = DEFAULT_PORT;
    if (argc > 1) {
        port = atoi(argv[1]);
        if (port <= 0 || port > 65535) {
            printf("Error: Invalid port number '%s'. Using default port %d.\n", argv[1], DEFAULT_PORT);
            port = DEFAULT_PORT;
        }
    }

    SOCKET serverSocket = create_server_socket(port);
    if (serverSocket == INVALID_SOCKET) {
        return 1;
    }

    run_server(serverSocket, port);

    closesocket(serverSocket);
    WSACleanup();
    return 0;
}