#ifndef SERVER_H
#define SERVER_H

#ifndef _WIN32_WINNT
    #define _WIN32_WINNT 0x0600
#endif

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

#define DEFAULT_PORT 80
#define BUFFER_SIZE 8192
#define PATH_MAX_LEN 512
#define MAX_THREADS 10
#define MAX_QUEUE 50

#ifndef INET_ADDRSTRLEN
    #define INET_ADDRSTRLEN 22
#endif

// 任务队列结构体
typedef struct {
    SOCKET clientSocket;
    struct sockaddr_in clientAddr;
} Task;

// 线程池结构体
typedef struct {
    HANDLE threads[MAX_THREADS];
    Task queue[MAX_QUEUE];
    int front, rear;
    HANDLE mutex;
    HANDLE semaphore;
    int shutdown;
} ThreadPool;

// 线程池函数
ThreadPool* create_thread_pool();
void destroy_thread_pool(ThreadPool* pool);
void add_task(ThreadPool* pool, SOCKET clientSocket, struct sockaddr_in clientAddr);

#endif