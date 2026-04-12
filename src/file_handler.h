#ifndef FILE_HANDLER_H
#define FILE_HANDLER_H

#include <stdio.h>
#include "server.h"

// 发送文件内容（已打开的文件）
void send_file_content(SOCKET client, FILE* file, const char* mime_type, long file_size);

// 发送目录列表页面
void send_directory_listing(SOCKET client, const char* dir_path, const char* request_uri);

#endif