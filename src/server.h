#ifndef SERVER_H
#define SERVER_H

#ifndef _WIN32_WINNT
    #define _WIN32_WINNT 0x0600
#endif

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>

#define DEFAULT_PORT 80
#define BUFFER_SIZE 8192
#define PATH_MAX_LEN 512

#ifndef INET_ADDRSTRLEN
    #define INET_ADDRSTRLEN 22
#endif

#endif