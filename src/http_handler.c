#include "http_handler.h"
#include "http_response.h"
#include "file_handler.h"
#include "utils.h"
#include <stdio.h>
#include <string.h>

// 辅助函数：打开UTF-8编码的文件路径
FILE* fopen_utf8(const char* path, const char* mode) {
    wchar_t wpath[PATH_MAX_LEN];
    wchar_t wmode[16];
    
    // 将UTF-8路径转换为宽字符
    MultiByteToWideChar(CP_UTF8, 0, path, -1, wpath, PATH_MAX_LEN);
    // 将模式字符串转换为宽字符
    MultiByteToWideChar(CP_UTF8, 0, mode, -1, wmode, 16);
    
    // 使用宽字符版本的fopen
    return _wfopen(wpath, wmode);
}

void handle_request(SOCKET client, const char* request) {
    char method[16], uri[PATH_MAX_LEN], protocol[16];
    char local_path[PATH_MAX_LEN];
    
    if (sscanf(request, "%s %s %s", method, uri, protocol) != 3) {
        send_404(client);
        return;
    }

    if (strcmp(method, "GET") != 0) {
        send_404(client);
        return;
    }

    if (strstr(uri, "..") != NULL) {
        send_404(client);
        return;
    }

    char decoded_uri[PATH_MAX_LEN];
    url_decode(uri, decoded_uri, sizeof(decoded_uri));
    
    if (strcmp(decoded_uri, "/") == 0) {
        strcpy(local_path, "./index.html");
    } else {
        sprintf(local_path, ".%s", decoded_uri);
    }

    FILE* file = fopen_utf8(local_path, "rb");
    
    if (file) {
        fseek(file, 0, SEEK_END);
        long file_size = ftell(file);
        rewind(file);

        const char* mime = get_mime_type(local_path);
        send_file_content(client, file, mime, file_size);
        fclose(file);
        return;
    }
    
    // 文件不存在，检查目录情况
    if (strcmp(decoded_uri, "/") == 0) {
        // 根目录且 index.html 不存在
        if (is_directory("./")) {
            send_directory_listing(client, ".", "/");
            return;
        }
    } else {
        char dir_path[PATH_MAX_LEN];
        sprintf(dir_path, ".%s", decoded_uri);
        
        if (is_directory(dir_path)) {
            size_t uri_len = strlen(decoded_uri);
            if (decoded_uri[uri_len-1] != '/') {
                char redirect_uri[PATH_MAX_LEN];
                sprintf(redirect_uri, "%s/", decoded_uri);
                send_redirect(client, redirect_uri);
                return;
            } else {
                send_directory_listing(client, dir_path, decoded_uri);
                return;
            }
        }
    }
    
    // 既非文件也非目录
    send_404(client);
}