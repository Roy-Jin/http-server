#ifndef UTILS_H
#define UTILS_H

#include "server.h"

// 终端颜色定义
#define COLOR_RESET   "\x1b[0m"
#define COLOR_RED     "\x1b[31m"
#define COLOR_GREEN   "\x1b[32m"
#define COLOR_YELLOW  "\x1b[33m"
#define COLOR_BLUE    "\x1b[34m"
#define COLOR_MAGENTA "\x1b[35m"
#define COLOR_CYAN    "\x1b[36m"
#define COLOR_WHITE   "\x1b[37m"

// 终端样式定义
#define STYLE_BOLD    "\x1b[1m"
#define STYLE_UNDERLINE "\x1b[4m"

// 组合样式
#define INFO_COLOR    COLOR_GREEN STYLE_BOLD
#define ERROR_COLOR   COLOR_RED STYLE_BOLD
#define WARNING_COLOR COLOR_YELLOW STYLE_BOLD
#define DEBUG_COLOR   COLOR_CYAN
#define HEADER_COLOR  COLOR_YELLOW STYLE_BOLD
#define VALUE_COLOR   COLOR_BLUE

// 获取文件的 MIME 类型
const char* get_mime_type(const char* path);

// 打印本机所有 IPv4 地址（非回环）
void print_local_ips(void);

// 检查路径是否为目录
int is_directory(const char* path);

// 格式化文件大小为人类可读的格式 (KB/MB/GB)
const char* format_file_size(long long bytes);

// URL解码函数，将URL编码的字符串解码为UTF-8字符串
void url_decode(const char* encoded, char* decoded, size_t max_len);

// 获取当前时间戳字符串
const char* get_timestamp(void);

#endif