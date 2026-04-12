#include "http_handler.h"
#include "http_response.h"
#include "file_handler.h"
#include "utils.h"
#include <stdio.h>
#include <string.h>

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

    if (strcmp(uri, "/") == 0) {
        strcpy(local_path, "./index.html");
    } else {
        sprintf(local_path, ".%s", uri);
    }

    FILE* file = fopen(local_path, "rb");
    
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
    if (strcmp(uri, "/") == 0) {
        // 根目录且 index.html 不存在
        if (is_directory("./")) {
            send_directory_listing(client, ".", "/");
            return;
        }
    } else {
        char dir_path[PATH_MAX_LEN];
        sprintf(dir_path, ".%s", uri);
        
        if (is_directory(dir_path)) {
            size_t uri_len = strlen(uri);
            if (uri[uri_len-1] != '/') {
                char redirect_uri[PATH_MAX_LEN];
                sprintf(redirect_uri, "%s/", uri);
                send_redirect(client, redirect_uri);
                return;
            } else {
                send_directory_listing(client, dir_path, uri);
                return;
            }
        }
    }
    
    // 既非文件也非目录
    send_404(client);
}