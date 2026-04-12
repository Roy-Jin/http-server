#ifndef HTTP_HANDLER_H
#define HTTP_HANDLER_H

#include "server.h"

// 辅助函数：打开UTF-8编码的文件路径
FILE* fopen_utf8(const char* path, const char* mode);

// 处理单个 HTTP 请求
void handle_request(SOCKET client, const char* request);

#endif