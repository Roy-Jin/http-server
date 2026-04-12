#ifndef HTTP_HANDLER_H
#define HTTP_HANDLER_H

#include "server.h"

// 处理单个 HTTP 请求
void handle_request(SOCKET client, const char* request);

#endif