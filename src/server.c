#include "server.h"
#include "http_handler.h"
#include "utils.h"
#include "config.h"
#include <stdio.h>
#include <stdlib.h>

#pragma comment(lib, "ws2_32.lib")

// 线程函数，处理客户端请求
DWORD WINAPI thread_function(LPVOID lpParam) {
    ThreadPool* pool = (ThreadPool*)lpParam;
    char buffer[BUFFER_SIZE];

    while (1) {
        // 等待信号量，获取任务
        WaitForSingleObject(pool->semaphore, INFINITE);

        // 检查是否需要关闭
        if (pool->shutdown) {
            break;
        }

        // 加锁获取任务
        WaitForSingleObject(pool->mutex, INFINITE);
        Task task = pool->queue[pool->front];
        pool->front = (pool->front + 1) % g_config.max_queue;
        ReleaseMutex(pool->mutex);

        // 处理请求
        memset(buffer, 0, BUFFER_SIZE);
        int bytesReceived = recv(task.clientSocket, buffer, BUFFER_SIZE - 1, 0);
        
        if (bytesReceived > 0) {
            handle_request(task.clientSocket, buffer);
        }

        // 关闭客户端连接
        closesocket(task.clientSocket);
    }

    return 0;
}

// 创建线程池
ThreadPool* create_thread_pool() {
    // 分配线程池内存
    ThreadPool* pool = (ThreadPool*)malloc(sizeof(ThreadPool));
    if (!pool) {
        printf("Error: Failed to allocate memory for thread pool.\n");
        return NULL;
    }

    // 初始化线程池结构
    pool->front = 0;
    pool->rear = 0;
    pool->shutdown = 0;

    // 创建互斥锁
    pool->mutex = CreateMutex(NULL, FALSE, NULL);
    if (!pool->mutex) {
        printf("Error: Failed to create mutex: %d\n", GetLastError());
        free(pool);
        return NULL;
    }

    // 创建信号量
    pool->semaphore = CreateSemaphore(NULL, 0, g_config.max_queue, NULL);
    if (!pool->semaphore) {
        printf("Error: Failed to create semaphore: %d\n", GetLastError());
        CloseHandle(pool->mutex);
        free(pool);
        return NULL;
    }

    // 创建工作线程
    for (int i = 0; i < g_config.max_threads; i++) {
        pool->threads[i] = CreateThread(NULL, 0, thread_function, pool, 0, NULL);
        if (!pool->threads[i]) {
            printf("Error: Failed to create thread %d: %d\n", i, GetLastError());
            
            // 清理已创建的线程
            for (int j = 0; j < i; j++) {
                CloseHandle(pool->threads[j]);
            }
            
            // 清理资源
            CloseHandle(pool->mutex);
            CloseHandle(pool->semaphore);
            free(pool);
            return NULL;
        }
    }

    return pool;
}

// 销毁线程池
void destroy_thread_pool(ThreadPool* pool) {
    if (!pool) {
        return;
    }

    // 标记关闭
    WaitForSingleObject(pool->mutex, INFINITE);
    pool->shutdown = 1;
    ReleaseMutex(pool->mutex);

    // 唤醒所有线程
    for (int i = 0; i < g_config.max_threads; i++) {
        ReleaseSemaphore(pool->semaphore, 1, NULL);
    }

    // 等待所有线程结束
    for (int i = 0; i < g_config.max_threads; i++) {
        WaitForSingleObject(pool->threads[i], INFINITE);
        CloseHandle(pool->threads[i]);
    }

    // 清理资源
    CloseHandle(pool->mutex);
    CloseHandle(pool->semaphore);
    free(pool);
}

// 添加任务到线程池
void add_task(ThreadPool* pool, SOCKET clientSocket, struct sockaddr_in clientAddr) {
    WaitForSingleObject(pool->mutex, INFINITE);
    
    // 检查队列是否已满
    int next = (pool->rear + 1) % g_config.max_queue;
    if (next == pool->front) {
        // 队列已满，关闭连接
        closesocket(clientSocket);
        ReleaseMutex(pool->mutex);
        return;
    }

    // 添加任务到队列
    pool->queue[pool->rear].clientSocket = clientSocket;
    pool->queue[pool->rear].clientAddr = clientAddr;
    pool->rear = next;

    ReleaseMutex(pool->mutex);
    
    // 信号通知线程有新任务
    ReleaseSemaphore(pool->semaphore, 1, NULL);
}

// 初始化 Winsock 并创建监听套接字
SOCKET create_server_socket(int port) {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("Error: WSAStartup failed.\n");
        return INVALID_SOCKET;
    }

    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == INVALID_SOCKET) {
        printf("Error: Socket creation failed: %d\n", WSAGetLastError());
        WSACleanup();
        return INVALID_SOCKET;
    }

    int opt = 1;
    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));

    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(port);

    if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        printf("Error: Bind failed: %d\n", WSAGetLastError());
        closesocket(serverSocket);
        WSACleanup();
        return INVALID_SOCKET;
    }

    if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR) {
        printf("Error: Listen failed: %d\n", WSAGetLastError());
        closesocket(serverSocket);
        WSACleanup();
        return INVALID_SOCKET;
    }

    return serverSocket;
}

void run_server(SOCKET serverSocket, int port) {
    print_local_ips();
    printf("  Port: %d\n", port);
    printf("  Access: http://0.0.0.0:%d\n", port);
    printf("  Press Ctrl+C to stop.\n\n");

    // 创建线程池
    ThreadPool* pool = create_thread_pool();
    if (!pool) {
        printf("Error: Failed to create thread pool.\n");
        return;
    }

    struct sockaddr_in clientAddr;
    int clientAddrLen = sizeof(clientAddr);

    while (1) {
        SOCKET clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &clientAddrLen);
        if (clientSocket != INVALID_SOCKET) {
            // 添加任务到线程池
            add_task(pool, clientSocket, clientAddr);
        }
    }

    // 销毁线程池（实际上不会执行到这里，因为是无限循环）
    destroy_thread_pool(pool);
}