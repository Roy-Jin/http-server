#include "file_handler.h"
#include "http_response.h"
#include "http_handler.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <winsock2.h>
#include <ws2tcpip.h>

// 全局缓存实例
FileCache g_file_cache = {
    .head = NULL,
    .max_items = 100,
    .current_items = 0,
    .max_size = 10 * 1024 * 1024, // 10MB
    .current_size = 0
};

// 初始化文件缓存
void init_file_cache(int max_items, long max_size) {
    g_file_cache.max_items = max_items;
    g_file_cache.max_size = max_size;
    g_file_cache.current_items = 0;
    g_file_cache.current_size = 0;
    g_file_cache.head = NULL;
}

// 清理文件缓存
void cleanup_file_cache() {
    CacheItem* current = g_file_cache.head;
    while (current) {
        CacheItem* next = current->next;
        free(current->content);
        free(current);
        current = next;
    }
    g_file_cache.head = NULL;
    g_file_cache.current_items = 0;
    g_file_cache.current_size = 0;
}

// 从缓存中获取文件
int get_cached_file(const char* path, char** content, long* size, time_t* last_modified) {
    CacheItem* current = g_file_cache.head;
    CacheItem* prev = NULL;
    
    while (current) {
        if (strcmp(current->path, path) == 0) {
            // 更新访问时间
            current->last_accessed = time(NULL);
            
            // 将当前项移到链表头部（LRU策略）
            if (prev) {
                prev->next = current->next;
                current->next = g_file_cache.head;
                g_file_cache.head = current;
            }
            
            *content = current->content;
            *size = current->size;
            *last_modified = current->last_modified;
            return 1;
        }
        prev = current;
        current = current->next;
    }
    
    return 0;
}

// 将文件添加到缓存
void add_to_cache(const char* path, const char* content, long size, time_t last_modified) {
    // 检查是否超过缓存大小限制
    while (g_file_cache.current_size + size > g_file_cache.max_size || 
           g_file_cache.current_items >= g_file_cache.max_items) {
        // 移除最久未使用的项（链表尾部）
        if (!g_file_cache.head) break;
        
        CacheItem* current = g_file_cache.head;
        CacheItem* prev = NULL;
        
        // 找到链表尾部
        while (current->next) {
            prev = current;
            current = current->next;
        }
        
        // 移除尾部项
        if (prev) {
            prev->next = NULL;
        } else {
            g_file_cache.head = NULL;
        }
        
        g_file_cache.current_size -= current->size;
        g_file_cache.current_items--;
        free(current->content);
        free(current);
    }
    
    // 创建新的缓存项
    CacheItem* new_item = (CacheItem*)malloc(sizeof(CacheItem));
    if (!new_item) return;
    
    strncpy(new_item->path, path, PATH_MAX_LEN - 1);
    new_item->path[PATH_MAX_LEN - 1] = '\0';
    new_item->content = (char*)malloc(size);
    if (!new_item->content) {
        free(new_item);
        return;
    }
    memcpy(new_item->content, content, size);
    new_item->size = size;
    new_item->last_modified = last_modified;
    new_item->last_accessed = time(NULL);
    new_item->next = g_file_cache.head;
    
    // 添加到链表头部
    g_file_cache.head = new_item;
    g_file_cache.current_size += size;
    g_file_cache.current_items++;
}

// 检查文件是否适合gzip压缩
int is_compressible(const char* mime_type) {
    // 文本类型适合压缩
    const char* compressible_types[] = {
        "text/",
        "application/javascript",
        "application/json",
        "application/xml",
        "application/css",
        NULL
    };
    
    for (int i = 0; compressible_types[i]; i++) {
        if (strncmp(mime_type, compressible_types[i], strlen(compressible_types[i])) == 0) {
            return 1;
        }
    }
    
    return 0;
}

// 发送文件内容（从路径）
void send_file_from_path(SOCKET client, const char* path, const char* mime_type, int gzip_supported) {
    // 检查文件是否在缓存中
    char* cached_content = NULL;
    long cached_size = 0;
    time_t cached_mtime = 0;
    
    if (get_cached_file(path, &cached_content, &cached_size, &cached_mtime)) {
        // 检查是否支持gzip且文件适合压缩
        if (gzip_supported && is_compressible(mime_type)) {
            // 压缩缓存内容
            char* compressed_content = NULL;
            size_t compressed_size = 0;
            if (gzip_compress(cached_content, cached_size, &compressed_content, &compressed_size)) {
                send_header_gzip(client, 200, "OK", mime_type, (long)compressed_size, cached_mtime);
                send(client, compressed_content, (int)compressed_size, 0);
                free(compressed_content);
                return;
            }
        }
        
        // 从缓存发送未压缩内容
        send_header(client, 200, "OK", mime_type, cached_size, cached_mtime);
        send(client, cached_content, (int)cached_size, 0);
        return;
    }
    
    // 打开文件
    FILE* file = fopen_utf8(path, "rb");
    if (!file) {
        send_404(client);
        return;
    }
    
    // 获取文件大小
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    rewind(file);
    
    // 对于小文件（小于1MB），读取到内存并缓存
    if (file_size < 1024 * 1024) {
        char* buffer = (char*)malloc(file_size);
        if (buffer) {
            size_t bytes_read = fread(buffer, 1, file_size, file);
            if (bytes_read == file_size) {
                // 获取文件最后修改时间
                wchar_t wpath[PATH_MAX_LEN];
                MultiByteToWideChar(CP_UTF8, 0, path, -1, wpath, PATH_MAX_LEN);
                struct _stat file_stat;
                time_t modified_time = 0;
                if (_wstat(wpath, &file_stat) == 0) {
                    modified_time = file_stat.st_mtime;
                    add_to_cache(path, buffer, file_size, modified_time);
                }
                
                // 检查是否支持gzip且文件适合压缩
                if (gzip_supported && is_compressible(mime_type)) {
                    // 压缩内容
                    char* compressed_content = NULL;
                    size_t compressed_size = 0;
                    if (gzip_compress(buffer, file_size, &compressed_content, &compressed_size)) {
                        send_header_gzip(client, 200, "OK", mime_type, (long)compressed_size, modified_time);
                        send(client, compressed_content, (int)compressed_size, 0);
                        free(compressed_content);
                        free(buffer);
                        fclose(file);
                        return;
                    }
                }
                
                // 发送未压缩内容
                send_header(client, 200, "OK", mime_type, file_size, modified_time);
                send(client, buffer, (int)file_size, 0);
                free(buffer);
                fclose(file);
                return;
            }
            free(buffer);
        }
    }
    
    // 获取文件最后修改时间
    wchar_t wpath[PATH_MAX_LEN];
    MultiByteToWideChar(CP_UTF8, 0, path, -1, wpath, PATH_MAX_LEN);
    struct _stat file_stat;
    time_t modified_time = 0;
    if (_wstat(wpath, &file_stat) == 0) {
        modified_time = file_stat.st_mtime;
    }
    
    // 对于大文件或缓存失败的情况，使用零拷贝传输
    send_file_content(client, file, mime_type, file_size, modified_time, gzip_supported);
    fclose(file);
}

// 发送部分文件内容（支持范围请求）
void send_partial_file_content(SOCKET client, FILE *file, const char *mime_type, long file_size, long start, long end) {
    // 发送206 Partial Content响应头
    char header[BUFFER_SIZE];
    int header_len = snprintf(header, BUFFER_SIZE,
        "HTTP/1.1 206 Partial Content\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %ld\r\n"
        "Content-Range: bytes %ld-%ld/%ld\r\n"
        "Accept-Ranges: bytes\r\n"
        "Connection: close\r\n"
        "\r\n",
        mime_type, end - start + 1, start, end, file_size);
    send(client, header, header_len, 0);

    // 使用传统的读写方式发送文件的一部分
    char buffer[BUFFER_SIZE];
    size_t bytes_read;
    fseek(file, start, SEEK_SET); // 设置文件指针到起始位置
    long bytes_remaining = end - start + 1;
    while (bytes_remaining > 0 && (bytes_read = fread(buffer, 1, BUFFER_SIZE, file)) > 0) {
        size_t send_size = bytes_read;
        if (send_size > bytes_remaining) {
            send_size = bytes_remaining;
        }
        send(client, buffer, (int)send_size, 0);
        bytes_remaining -= send_size;
    }
}

void send_file_content(SOCKET client, FILE *file, const char *mime_type, long file_size, time_t modified_time, int gzip_supported) {
    // 检查是否支持gzip且文件适合压缩
    if (gzip_supported && is_compressible(mime_type) && file_size < 1024 * 1024) { // 只对小文件进行压缩
        // 读取整个文件到内存
        char* buffer = (char*)malloc(file_size);
        if (buffer) {
            size_t bytes_read = fread(buffer, 1, file_size, file);
            if (bytes_read == file_size) {
                // 压缩内容
                char* compressed_content = NULL;
                size_t compressed_size = 0;
                if (gzip_compress(buffer, file_size, &compressed_content, &compressed_size)) {
                    send_header_gzip(client, 200, "OK", mime_type, (long)compressed_size, modified_time);
                    send(client, compressed_content, (int)compressed_size, 0);
                    free(compressed_content);
                    free(buffer);
                    return;
                }
            }
            free(buffer);
        }
    }
    
    // 发送未压缩内容
    send_header(client, 200, "OK", mime_type, file_size, modified_time);

    // 使用传统的读写方式发送文件
    char buffer[BUFFER_SIZE];
    size_t bytes_read;
    rewind(file); // 重置文件指针
    while ((bytes_read = fread(buffer, 1, BUFFER_SIZE, file)) > 0) {
        send(client, buffer, (int)bytes_read, 0);
    }
}

// 辅助函数：将宽字符字符串转换为UTF-8字符串
int wchar_to_utf8(const wchar_t* wstr, char* utf8, size_t max_len) {
    int len = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, utf8, (int)max_len, NULL, NULL);
    if (len == 0) {
        DWORD error = GetLastError();
        printf("Error in WideCharToMultiByte: %d\n", error);
        utf8[0] = '\0';
        return 0;
    }
    return len - 1; // 返回实际字符数，不包括终止符
}

void send_directory_listing(SOCKET client, const char* dir_path, const char* request_uri) {
    WIN32_FIND_DATAW findData;
    HANDLE hFind;
    wchar_t searchPath[PATH_MAX_LEN];
    char* html = NULL;
    size_t html_size = 0;
    size_t html_capacity = 32768; // 增加初始缓冲区大小，减少realloc次数
    
    // 将窄字符路径转换为宽字符
    MultiByteToWideChar(CP_UTF8, 0, dir_path, -1, searchPath, PATH_MAX_LEN);
    wcscat(searchPath, L"\\*");
    
    hFind = FindFirstFileW(searchPath, &findData);
    if (hFind == INVALID_HANDLE_VALUE) {
        send_404(client);
        return;
    }
    
    // 预分配足够大的缓冲区
    html = (char*)malloc(html_capacity);
    if (!html) {
        FindClose(hFind);
        send_404(client);
        return;
    }
    
    int len = snprintf(html, html_capacity,
        "<!DOCTYPE html>\n"
        "<html lang=\"en\">\n"
        "<head>\n"
        "    <meta charset=\"UTF-8\">\n"
        "    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n"
        "    <title>Directory: %s</title>\n"
        "    <style>\n"
        "        * {\n"
        "            margin: 0;\n"
        "            padding: 0;\n"
        "            box-sizing: border-box;\n"
        "        }\n"
        "        html, body {\n"
        "            height: 100%%;\n"
        "            width: 100%%;\n"
        "            overflow: hidden;\n"
        "        }\n"
        "        body {\n"
        "            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, 'Helvetica Neue', Arial, sans-serif;\n"
        "            background: linear-gradient(135deg, #0f0f0f 0%%, #1a1a2e 50%%, #16213e 100%%);\n"
        "            color: #e8e8e8;\n"
        "            display: flex;\n"
        "            flex-direction: column;\n"
        "        }\n"
        "        .container {\n"
        "            flex: 1;\n"
        "            display: flex;\n"
        "            flex-direction: column;\n"
        "            height: 100%%;\n"
        "            overflow: hidden;\n"
        "        }\n"
        "        .header {\n"
        "            background: rgba(20, 20, 30, 0.95);\n"
        "            backdrop-filter: blur(20px);\n"
        "            border-bottom: 1px solid rgba(255, 255, 255, 0.08);\n"
        "            padding: 24px 40px;\n"
        "            flex-shrink: 0;\n"
        "        }\n"
        "        h1 {\n"
        "            font-size: 28px;\n"
        "            font-weight: 700;\n"
        "            color: #ffffff;\n"
        "            margin-bottom: 8px;\n"
        "            letter-spacing: -0.5px;\n"
        "            display: flex;\n"
        "            align-items: center;\n"
        "            gap: 12px;\n"
        "        }\n"
        "        h1::before {\n"
        "            content: '';\n"
        "            display: inline-block;\n"
        "            width: 4px;\n"
        "            height: 28px;\n"
        "            background: linear-gradient(180deg, #667eea 0%%, #764ba2 100%%);\n"
        "            border-radius: 2px;\n"
        "        }\n"
        "        .path {\n"
        "            color: #888;\n"
        "            font-size: 14px;\n"
        "            font-family: 'SF Mono', 'Fira Code', 'Consolas', monospace;\n"
        "            padding-left: 16px;\n"
        "            word-break: break-all;\n"
        "        }\n"
        "        .content-wrapper {\n"
        "            flex: 1;\n"
        "            overflow-y: auto;\n"
        "            overflow-x: hidden;\n"
        "            padding: 0;\n"
        "        }\n"
        "        .directory-table {\n"
        "            padding: 16px 24px;\n"
        "        }\n"
        "        .file-item {\n"
        "            display: flex;\n"
        "            align-items: center;\n"
        "            padding: 14px 20px;\n"
        "            margin-bottom: 4px;\n"
        "            background: rgba(30, 30, 40, 0.6);\n"
        "            border-radius: 12px;\n"
        "            cursor: pointer;\n"
        "            transition: all 0.2s cubic-bezier(0.4, 0, 0.2, 1);\n"
        "            border: 1px solid transparent;\n"
        "        }\n"
        "        .file-item:hover {\n"
        "            background: rgba(50, 50, 70, 0.8);\n"
        "            border-color: rgba(102, 126, 234, 0.3);\n"
        "            transform: translateX(4px);\n"
        "        }\n"
        "        .file-item:active {\n"
        "            transform: translateX(2px);\n"
        "        }\n"
        "        .file-item .name-cell {\n"
        "            flex: 1;\n"
        "            display: flex;\n"
        "            align-items: center;\n"
        "            gap: 14px;\n"
        "            min-width: 0;\n"
        "        }\n"
        "        .file-item .icon {\n"
        "            font-size: 20px;\n"
        "            width: 28px;\n"
        "            text-align: center;\n"
        "            flex-shrink: 0;\n"
        "        }\n"
        "        .file-item .file-link,\n"
        "        .file-item .dir-link,\n"
        "        .file-item .parent-link {\n"
        "            color: #e8e8e8;\n"
        "            text-decoration: none;\n"
        "            font-weight: 500;\n"
        "            font-size: 15px;\n"
        "            white-space: nowrap;\n"
        "            overflow: hidden;\n"
        "            text-overflow: ellipsis;\n"
        "        }\n"
        "        .file-item:hover .file-link,\n"
        "        .file-item:hover .dir-link,\n"
        "        .file-item:hover .parent-link {\n"
        "            color: #667eea;\n"
        "        }\n"
        "        .file-item .size {\n"
        "            color: #666;\n"
        "            font-family: 'SF Mono', 'Fira Code', 'Consolas', monospace;\n"
        "            font-size: 13px;\n"
        "            padding-left: 20px;\n"
        "            flex-shrink: 0;\n"
        "            min-width: 80px;\n"
        "            text-align: right;\n"
        "        }\n"
        "        .file-item .type {\n"
        "            color: #667eea;\n"
        "            font-size: 12px;\n"
        "            font-weight: 600;\n"
        "            padding-left: 20px;\n"
        "            flex-shrink: 0;\n"
        "            text-transform: uppercase;\n"
        "            letter-spacing: 0.5px;\n"
        "        }\n"
        "        .folder-icon { color: #667eea; }\n"
        "        .file-icon { color: #48bb78; }\n"
        "        .parent-icon { color: #ed8936; }\n"
        "        .footer {\n"
        "            background: rgba(20, 20, 30, 0.95);\n"
        "            backdrop-filter: blur(20px);\n"
        "            border-top: 1px solid rgba(255, 255, 255, 0.08);\n"
        "            padding: 16px 40px;\n"
        "            text-align: center;\n"
        "            flex-shrink: 0;\n"
        "        }\n"
        "        .author {\n"
        "            color: #666;\n"
        "            font-size: 13px;\n"
        "        }\n"
        "        .author a {\n"
        "            color: #667eea;\n"
        "            text-decoration: none;\n"
        "            font-weight: 500;\n"
        "            transition: color 0.2s;\n"
        "        }\n"
        "        .author a:hover {\n"
        "            color: #764ba2;\n"
        "        }\n"
        "        ::-webkit-scrollbar {\n"
        "            width: 8px;\n"
        "        }\n"
        "        ::-webkit-scrollbar-track {\n"
        "            background: rgba(0, 0, 0, 0.2);\n"
        "        }\n"
        "        ::-webkit-scrollbar-thumb {\n"
        "            background: rgba(102, 126, 234, 0.3);\n"
        "            border-radius: 4px;\n"
        "        }\n"
        "        ::-webkit-scrollbar-thumb:hover {\n"
        "            background: rgba(102, 126, 234, 0.5);\n"
        "        }\n"
        "        @media (max-width: 768px) {\n"
        "            .header { padding: 20px 24px; }\n"
        "            .directory-table { padding: 12px 16px; }\n"
        "            .file-item { padding: 12px 16px; }\n"
        "            h1 { font-size: 22px; }\n"
        "            .file-item .file-link,\n"
        "            .file-item .dir-link,\n"
        "            .file-item .parent-link { font-size: 14px; }\n"
        "        }\n"
        "        @media (max-width: 480px) {\n"
        "            .header { padding: 16px 20px; }\n"
        "            .directory-table { padding: 8px 12px; }\n"
        "            .file-item { padding: 10px 14px; margin-bottom: 2px; }\n"
        "            h1 { font-size: 18px; }\n"
        "            .path { font-size: 12px; }\n"
        "            .file-item .icon { font-size: 18px; width: 24px; }\n"
        "            .file-item .file-link,\n"
        "            .file-item .dir-link,\n"
        "            .file-item .parent-link { font-size: 13px; }\n"
        "            .file-item .size { font-size: 11px; min-width: 60px; }\n"
        "            .file-item .type { font-size: 10px; }\n"
        "            .footer { padding: 12px 20px; }\n"
        "            .author { font-size: 11px; }\n"
        "        }\n"
        "    </style>\n"
        "</head>\n"
        "<body>\n"
        "    <div class=\"container\">\n"
        "        <div class=\"header\">\n"
        "            <h1>HTTP SERVER</h1>\n"
        "            <div class=\"path\">%s</div>\n"
        "        </div>\n"
        "        <div class=\"content-wrapper\">\n"
        "            <div class=\"directory-table\">\n",
        request_uri, request_uri);
    
    if (len < 0 || (size_t)len >= html_capacity) {
        free(html);
        FindClose(hFind);
        send_404(client);
        return;
    }
    html_size = len;
    
    // 父目录链接
    if (strcmp(request_uri, "/") != 0) {
        const char* parent_link = "<div class=\"file-item\"><div class=\"name-cell\"><span class=\"icon parent-icon\">⬆</span><span class=\"parent-link\" data-path=\"../\">..</span></div><span class=\"type\">DIR</span></div>\n";
        size_t add_len = strlen(parent_link);
        
        if (html_size + add_len + 1 > html_capacity) {
            html_capacity = html_size + add_len + 16384;
            char* new_html = (char*)realloc(html, html_capacity);
            if (!new_html) {
                free(html);
                FindClose(hFind);
                send_404(client);
                return;
            }
            html = new_html;
        }
        
        memcpy(html + html_size, parent_link, add_len);
        html_size += add_len;
    }
    
    do {
        if (wcscmp(findData.cFileName, L".") == 0 || wcscmp(findData.cFileName, L"..") == 0)
            continue;
        
        char fileName[PATH_MAX_LEN];
        wchar_to_utf8(findData.cFileName, fileName, sizeof(fileName));
        
        int isDir = (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
        
        size_t needed_space;
        if (isDir) {
            needed_space = strlen(fileName) * 2 + 150;
        } else {
            LARGE_INTEGER fileSize;
            fileSize.LowPart = findData.nFileSizeLow;
            fileSize.HighPart = findData.nFileSizeHigh;
            const char* size_str = format_file_size(fileSize.QuadPart);
            needed_space = strlen(fileName) * 2 + strlen(size_str) + 150;
        }
        
        if (html_size + needed_space + 1 > html_capacity) {
            html_capacity = html_size + needed_space + 16384;
            char* new_html = (char*)realloc(html, html_capacity);
            if (!new_html) {
                free(html);
                FindClose(hFind);
                send_404(client);
                return;
            }
            html = new_html;
        }
        
        if (isDir) {
            html_size += snprintf(html + html_size, html_capacity - html_size,
                "<div class=\"file-item\"><div class=\"name-cell\"><span class=\"icon folder-icon\">📁</span><span class=\"dir-link\" data-path=\"%s/\">%s</span></div><span class=\"type\">DIR</span></div>\n",
                fileName, fileName);
        } else {
            LARGE_INTEGER fileSize;
            fileSize.LowPart = findData.nFileSizeLow;
            fileSize.HighPart = findData.nFileSizeHigh;
            html_size += snprintf(html + html_size, html_capacity - html_size,
                "<div class=\"file-item\"><div class=\"name-cell\"><span class=\"icon file-icon\">📄</span><span class=\"file-link\" data-path=\"%s\">%s</span></div><span class=\"size\">%s</span></div>\n",
                fileName, fileName, format_file_size(fileSize.QuadPart));
        }
        
    } while (FindNextFileW(hFind, &findData));
    
    FindClose(hFind);
    
    const char* footer_and_script = 
        "            </div>\n"
        "        </div>\n"
        "        <div class=\"footer\">\n"
        "            <div class=\"author\">Created by <a href=\"https://github.com/Roy-Jin\" target=\"_blank\">GitHub/Roy-Jin</a></div>\n"
        "        </div>\n"
        "    </div>\n"
        "    <script>\n"
        "        (function() {\n"
        "            var wrapper = document.querySelector('.directory-table');\n"
        "            if (!wrapper) return;\n"
        "            wrapper.addEventListener('click', function(e) {\n"
        "                var item = e.target.closest('.file-item');\n"
        "                if (!item) return;\n"
        "                var linkElem = item.querySelector('[data-path]');\n"
        "                if (!linkElem) return;\n"
        "                var path = linkElem.getAttribute('data-path');\n"
        "                if (path) {\n"
        "                    window.location.href = path;\n"
        "                }\n"
        "            });\n"
    "        })();\n"
    "    </script>\n"
    "</body>\n"
    "</html>";
    
    size_t footer_len = strlen(footer_and_script);
    
    // 检查缓冲区大小并按需扩展
    if (html_size + footer_len + 1 > html_capacity) {
        html_capacity = html_size + footer_len + 1;
        char* new_html = (char*)realloc(html, html_capacity);
        if (!new_html) {
            free(html);
            send_404(client);
            return;
        }
        html = new_html;
    }
    
    // 直接使用memcpy复制字符串，提高效率
    memcpy(html + html_size, footer_and_script, footer_len);
    html_size += footer_len;
    
    send_header(client, 200, "OK", "text/html; charset=utf-8", (long)html_size, time(NULL));
    send(client, html, (int)html_size, 0);
    
    free(html);
}