#include "http_handler.h"
#include "http_response.h"
#include "file_handler.h"
#include "utils.h"
#include "config.h"
#include <stdio.h>
#include <string.h>

// 解析Range请求头
int parse_range_header(const char* request, long file_size, long* start, long* end) {
    const char* range_start = strstr(request, "Range:");
    if (!range_start) {
        return 0; // 没有Range头
    }
    
    range_start += 6; // 跳过"Range: "
    
    // 检查是否是bytes范围请求
    if (strncmp(range_start, "bytes=", 6) != 0) {
        return 0;
    }
    
    range_start += 6; // 跳过"bytes="
    
    // 解析范围
    char* dash = strchr(range_start, '-');
    if (!dash) {
        return 0;
    }
    
    // 解析起始位置
    *start = atol(range_start);
    
    // 解析结束位置
    if (dash[1] != '\0') {
        *end = atol(dash + 1);
    } else {
        *end = file_size - 1;
    }
    
    // 验证范围
    if (*start < 0 || *start >= file_size || *end < *start || *end >= file_size) {
        return 0;
    }
    
    return 1;
}

// 检查客户端是否支持gzip压缩
int is_gzip_supported(const char* request) {
    const char* accept_encoding = strstr(request, "Accept-Encoding:");
    if (!accept_encoding) {
        return 0; // 没有Accept-Encoding头
    }
    
    accept_encoding += 16; // 跳过"Accept-Encoding: "
    
    // 检查是否包含gzip
    while (*accept_encoding) {
        // 跳过空格
        while (*accept_encoding == ' ') {
            accept_encoding++;
        }
        
        // 检查是否是gzip
        if (strncmp(accept_encoding, "gzip", 4) == 0) {
            return 1;
        }
        
        // 找到下一个编码
        char* comma = strchr(accept_encoding, ',');
        if (!comma) {
            break;
        }
        accept_encoding = comma + 1;
    }
    
    return 0;
}

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

// 解析HTTP请求行
int parse_http_request(const char* request, char* method, char* uri, char* protocol) {
    return sscanf(request, "%s %s %s", method, uri, protocol) == 3;
}

// 验证请求合法性
int validate_request(const char* method, const char* uri) {
    // 只支持GET方法
    if (strcmp(method, "GET") != 0) {
        return 0;
    }
    
    // 防止路径遍历攻击
    if (strstr(uri, "..") != NULL) {
        return 0;
    }
    
    return 1;
}

// 处理索引文件查找
int handle_index_files(SOCKET client, const char* dir_path, const char* request) {
    char local_path[PATH_MAX_LEN];
    
    // 尝试查找索引文件
    for (int i = 0; i < 5 && g_config.index_files[i][0]; i++) {
        sprintf(local_path, "%s/%s", dir_path, g_config.index_files[i]);
        FILE* file = fopen_utf8(local_path, "rb");
        if (file) {
            fclose(file);
            const char* mime = get_mime_type(local_path);
            int gzip_supported = g_config.gzip && is_gzip_supported(request);
            send_file_from_path(client, local_path, mime, gzip_supported);
            return 1;
        }
    }
    
    return 0;
}

// 处理文件请求
int handle_file_request(SOCKET client, const char* root_dir, const char* decoded_uri, const char* request) {
    char local_path[PATH_MAX_LEN];
    
    sprintf(local_path, "%s%s", root_dir, decoded_uri);
    
    // 检查文件是否存在
    FILE* file = fopen_utf8(local_path, "rb");
    if (file) {
        // 获取文件大小
        fseek(file, 0, SEEK_END);
        long file_size = ftell(file);
        rewind(file);
        
        // 检查是否有Range请求
        long start = 0, end = 0;
        const char* mime = get_mime_type(local_path);
        int gzip_supported = g_config.gzip && is_gzip_supported(request);
        
        if (parse_range_header(request, file_size, &start, &end)) {
            // 处理范围请求
            send_partial_file_content(client, file, mime, file_size, start, end);
        } else {
            // 正常请求，使用缓存机制
            fclose(file);
            send_file_from_path(client, local_path, mime, gzip_supported);
            return 1;
        }
        
        fclose(file);
        return 1;
    }
    
    return 0;
}

// 处理目录请求
int handle_directory_request(SOCKET client, const char* root_dir, const char* decoded_uri, const char* request) {
    char dir_path[PATH_MAX_LEN];
    sprintf(dir_path, "%s%s", root_dir, decoded_uri);
    
    if (is_directory(dir_path)) {
        size_t uri_len = strlen(decoded_uri);
        if (decoded_uri[uri_len-1] != '/') {
            // 重定向到带斜杠的URL
            char redirect_uri[PATH_MAX_LEN];
            sprintf(redirect_uri, "%s/", decoded_uri);
            send_redirect(client, redirect_uri);
            return 1;
        } else {
            // 尝试查找索引文件
            if (handle_index_files(client, dir_path, request)) {
                return 1;
            }
            
            // 索引文件不存在，显示目录列表
            if (g_config.directory_listing) {
                send_directory_listing(client, dir_path, decoded_uri);
                return 1;
            } else {
                send_404(client);
                return 1;
            }
        }
    }
    
    return 0;
}

void handle_request(SOCKET client, const char* request) {
    char method[16], uri[PATH_MAX_LEN], protocol[16];
    
    // 解析请求
    if (!parse_http_request(request, method, uri, protocol)) {
        send_404(client);
        return;
    }
    
    // 记录请求日志
    printf("[%s] %s %s\n", get_timestamp(), method, uri);

    // 验证请求
    if (!validate_request(method, uri)) {
        send_404(client);
        return;
    }

    // URL解码
    char decoded_uri[PATH_MAX_LEN];
    url_decode(uri, decoded_uri, sizeof(decoded_uri));
    
    // 处理根目录请求
    if (strcmp(decoded_uri, "/") == 0) {
        // 尝试处理索引文件
        if (handle_index_files(client, g_config.root_dir, request)) {
            return;
        }
        
        // 索引文件不存在，检查是否为目录
        if (is_directory(g_config.root_dir)) {
            if (g_config.directory_listing) {
                send_directory_listing(client, g_config.root_dir, "/");
                return;
            } else {
                send_404(client);
                return;
            }
        }
    } else {
        // 处理非根目录请求
        if (handle_file_request(client, g_config.root_dir, decoded_uri, request)) {
            return;
        }
        
        // 文件不存在，检查是否为目录
        if (handle_directory_request(client, g_config.root_dir, decoded_uri, request)) {
            return;
        }
    }
    
    // 既非文件也非目录
    send_404(client);
}