#ifndef FILE_HANDLER_H
#define FILE_HANDLER_H

#include <stdio.h>
#include <time.h>
#include "server.h"

// 缓存项结构体
typedef struct CacheItem {
    char path[PATH_MAX_LEN];      // 文件路径
    char* content;                 // 文件内容
    long size;                     // 文件大小
    time_t last_modified;          // 最后修改时间
    time_t last_accessed;          // 最后访问时间
    struct CacheItem* next;        // 链表下一项
} CacheItem;

// 缓存管理结构体
typedef struct {
    CacheItem* head;               // 缓存链表头
    int max_items;                 // 最大缓存项数
    int current_items;             // 当前缓存项数
    long max_size;                 // 最大缓存大小（字节）
    long current_size;             // 当前缓存大小（字节）
} FileCache;

// 全局缓存实例
extern FileCache g_file_cache;

// 初始化文件缓存
void init_file_cache(int max_items, long max_size);

// 清理文件缓存
void cleanup_file_cache();

// 从缓存中获取文件
int get_cached_file(const char* path, char** content, long* size, time_t* last_modified);

// 将文件添加到缓存
void add_to_cache(const char* path, const char* content, long size, time_t last_modified);

// 发送文件内容（已打开的文件）
void send_file_content(SOCKET client, FILE* file, const char* mime_type, long file_size);

// 发送部分文件内容（支持范围请求）
void send_partial_file_content(SOCKET client, FILE* file, const char* mime_type, long file_size, long start, long end);

// 发送文件内容（从路径）
void send_file_from_path(SOCKET client, const char* path, const char* mime_type);

// 发送目录列表页面
void send_directory_listing(SOCKET client, const char* dir_path, const char* request_uri);

#endif