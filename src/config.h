#ifndef CONFIG_H
#define CONFIG_H

#include <stdio.h>
#include <stdbool.h>

// 版本号定义
#define SERVER_VERSION "2.1.1"
#define SERVER_NAME "http-server"

// 配置结构体
typedef struct {
    int port;               // 服务器端口
    char root_dir[512];     // 根目录路径
    bool directory_listing; // 是否启用目录列表
    int max_threads;        // 最大线程数
    int max_queue;          // 最大队列长度
    bool cors;              // 是否启用CORS
    bool gzip;              // 是否启用GZip
    char index_files[5][128]; // 索引文件列表
} Config;

// 全局配置实例
extern Config g_config;

// 配置初始化
bool config_init(int argc, char* argv[]);

// 显示当前配置
void config_show();

#endif