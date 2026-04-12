#include "src/server.h"
#include "src/config.h"
#include <stdio.h>
#include <stdlib.h>

SOCKET create_server_socket(int port);
void run_server(SOCKET serverSocket, int port);

int main(int argc, char* argv[]) {
    printf("\x1b[32m\x1b[47m\x1b[1m Created by Roy-Jin. \x1b[0m\n");

    // 初始化配置
    if (!config_init(argc, argv)) {
        return 1;
    }

    // 显示当前配置
    config_show();

    SOCKET serverSocket = create_server_socket(g_config.port);
    if (serverSocket == INVALID_SOCKET) {
        return 1;
    }

    run_server(serverSocket, g_config.port);

    closesocket(serverSocket);
    WSACleanup();
    return 0;
}