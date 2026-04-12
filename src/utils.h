#ifndef UTILS_H
#define UTILS_H

#include "server.h"

// 获取文件的 MIME 类型
const char* get_mime_type(const char* path);

// 打印本机所有 IPv4 地址（非回环）
void print_local_ips(void);

// 检查路径是否为目录
int is_directory(const char* path);

// 格式化文件大小为人类可读的格式 (KB/MB/GB)
const char* format_file_size(long long bytes);

#endif