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

// 发送文件内容（从路径）
void send_file_from_path(SOCKET client, const char* path, const char* mime_type) {
    // 检查文件是否在缓存中
    char* cached_content = NULL;
    long cached_size = 0;
    time_t cached_mtime = 0;
    
    if (get_cached_file(path, &cached_content, &cached_size, &cached_mtime)) {
        // 从缓存发送
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
                if (_wstat(wpath, &file_stat) == 0) {
                    add_to_cache(path, buffer, file_size, file_stat.st_mtime);
                }
                
                // 发送文件内容
                send_header(client, 200, "OK", mime_type, file_size, file_stat.st_mtime);
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
    send_file_content(client, file, mime_type, file_size, modified_time);
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

void send_file_content(SOCKET client, FILE *file, const char *mime_type, long file_size, time_t modified_time) {
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
    
    // 使用snprintf直接写入缓冲区，避免字符串复制
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
        "        body {\n"
        "            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, Oxygen, Ubuntu, sans-serif;\n"
        "            background: linear-gradient(135deg, #f5f7fa 0%%, #c3cfe2 100%%);\n"
        "            min-height: 100vh;\n"
        "            padding: 20px;\n"
        "            color: #333;\n"
        "        }\n"
        "        .container {\n"
        "            max-width: 1000px;\n"
        "            margin: 0 auto;\n"
        "        }\n"
        "        .header {\n"
        "            background: white;\n"
        "            border-radius: 15px 15px 0 0;\n"
        "            padding: 30px;\n"
        "            box-shadow: 0 5px 15px rgba(0, 0, 0, 0.08);\n"
        "            margin-bottom: 2px;\n"
        "        }\n"
        "        h1 {\n"
        "            font-size: 28px;\n"
        "            font-weight: 600;\n"
        "            color: #2c3e50;\n"
        "            margin-bottom: 10px;\n"
        "        }\n"
        "        .path {\n"
        "            color: #7f8c8d;\n"
        "            font-size: 16px;\n"
        "            word-break: break-all;\n"
        "        }\n"
        "        .directory-table {\n"
        "            background: white;\n"
        "            border-radius: 0 0 15px 15px;\n"
        "            overflow: hidden;\n"
        "            box-shadow: 0 5px 15px rgba(0, 0, 0, 0.08);\n"
        "        }\n"
        "        table {\n"
        "            width: 100%%;\n"
        "            border-collapse: collapse;\n"
        "        }\n"
        "        thead {\n"
        "            background: #f8f9fa;\n"
        "        }\n"
        "        th {\n"
        "            text-align: left;\n"
        "            padding: 18px 20px;\n"
        "            font-weight: 600;\n"
        "            color: #2c3e50;\n"
        "            border-bottom: 2px solid #e9ecef;\n"
        "        }\n"
        "        tbody tr {\n"
        "            border-bottom: 1px solid #f1f3f4;\n"
        "            transition: background-color 0.2s ease;\n"
        "            cursor: pointer;\n"
        "        }\n"
        "        tbody tr:hover {\n"
        "            background-color: #f8f9fa;\n"
        "        }\n"
        "        td {\n"
        "            padding: 16px 20px;\n"
        "            vertical-align: middle;\n"
        "        }\n"
        "        .file-link, .dir-link, .parent-link {\n"
        "            color: #4285f4;\n"
        "            text-decoration: none;\n"
        "            font-weight: 500;\n"
        "            display: inline-block;\n"
        "            padding: 8px 0;\n"
        "            min-height: 44px;\n"
        "            line-height: 28px;\n"
        "        }\n"
        "        .name-cell {\n"
        "            display: flex;\n"
        "            align-items: center;\n"
        "            gap: 12px;\n"
        "        }\n"
        "        .icon {\n"
        "            font-size: 20px;\n"
        "            width: 24px;\n"
        "            text-align: center;\n"
        "        }\n"
        "        .folder-icon {\n"
        "            color: #4285f4;\n"
        "        }\n"
        "        .file-icon {\n"
        "            color: #34a853;\n"
        "        }\n"
        "        .parent-icon {\n"
        "            color: #ea4335;\n"
        "        }\n"
        "        .size {\n"
        "            color: #5f6368;\n"
        "            font-family: 'Courier New', monospace;\n"
        "            font-size: 14px;\n"
        "        }\n"
        "        .type {\n"
        "            color: #ea4335;\n"
        "            font-size: 14px;\n"
        "            font-weight: 500;\n"
        "        }\n"
        "        .author {\n"
        "            text-align: center;\n"
        "            margin-top: 20px;\n"
        "            color: #666;\n"
        "            font-size: 14px;\n"
        "        }\n"
        "        .author a {\n"
        "            color: #4285f4;\n"
        "            text-decoration: none;\n"
        "        }\n"
        "        .author a:hover {\n"
        "            text-decoration: underline;\n"
        "        }\n"
        "        @media (max-width: 768px) {\n"
        "            .container {\n"
        "                padding: 10px;\n"
        "            }\n"
        "            .header {\n"
        "                padding: 20px;\n"
        "            }\n"
        "            h1 {\n"
        "                font-size: 22px;\n"
        "            }\n"
        "            th, td {\n"
        "                padding: 12px 15px;\n"
        "            }\n"
        "            .directory-table {\n"
        "                overflow-x: auto;\n"
        "            }\n"
        "            table {\n"
        "                min-width: 400px;\n"
        "            }\n"
        "        }\n"
        "        @media (max-width: 480px) {\n"
        "            body {\n"
        "                padding: 10px;\n"
        "            }\n"
        "            .header {\n"
        "                padding: 16px;\n"
        "            }\n"
        "            h1 {\n"
        "                font-size: 20px;\n"
        "            }\n"
        "            .path {\n"
        "                font-size: 14px;\n"
        "            }\n"
        "            th, td {\n"
        "                padding: 10px 12px;\n"
        "            }\n"
        "            .name-cell {\n"
        "                gap: 8px;\n"
        "            }\n"
        "            .icon {\n"
        "                font-size: 18px;\n"
        "                width: 20px;\n"
        "            }\n"
        "            .file-link, .dir-link, .parent-link {\n"
        "                font-size: 14px;\n"
        "            }\n"
        "            .size, .type {\n"
        "                font-size: 12px;\n"
        "            }\n"
        "        }\n"
        "    </style>\n"
        "</head>\n"
        "<body>\n"
        "    <div class=\"container\">\n"
        "        <div class=\"header\">\n"
        "            <h1>📁 HTTP SERVER</h1>\n"
        "            <div class=\"path\">Path: %s</div>\n"
        "        </div>\n"
        "        <div class=\"directory-table\">\n"
        "            <table>\n"
        "                <thead>\n"
        "                    <tr>\n"
        "                        <th>Name</th>\n"
        "                        <th>Size/Type</th>\n"
        "                    </tr>\n"
        "                </thead>\n"
        "                <tbody>\n",
        request_uri, request_uri);
    
    if (len < 0 || (size_t)len >= html_capacity) {
        free(html);
        FindClose(hFind);
        send_404(client);
        return;
    }
    html_size = len;
    
    // 父目录链接（使用 span + data-path）
    if (strcmp(request_uri, "/") != 0) {
        const char* parent_link = "<tr><td class=\"name-cell\"><span class=\"icon parent-icon\">↩</span><span class=\"parent-link\" data-path=\"../\">Parent Directory</span></td><td class=\"type\">&lt;dir&gt;</td></tr>\n";
        size_t add_len = strlen(parent_link);
        
        // 检查缓冲区大小并按需扩展
        if (html_size + add_len + 1 > html_capacity) {
            // 一次性扩展更大的空间，减少后续realloc
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
        
        // 直接使用memcpy复制字符串，提高效率
        memcpy(html + html_size, parent_link, add_len);
        html_size += add_len;
    }
    
    do {
        if (wcscmp(findData.cFileName, L".") == 0 || wcscmp(findData.cFileName, L"..") == 0)
            continue;
        
        char fileName[PATH_MAX_LEN];
        wchar_to_utf8(findData.cFileName, fileName, sizeof(fileName));
        
        int isDir = (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
        
        // 计算所需空间，避免频繁realloc
        size_t needed_space;
        if (isDir) {
            needed_space = strlen(fileName) * 2 + 100; // 估算目录项所需空间
        } else {
            LARGE_INTEGER fileSize;
            fileSize.LowPart = findData.nFileSizeLow;
            fileSize.HighPart = findData.nFileSizeHigh;
            const char* size_str = format_file_size(fileSize.QuadPart);
            needed_space = strlen(fileName) * 2 + strlen(size_str) + 100; // 估算文件项所需空间
        }
        
        // 检查缓冲区大小并按需扩展
        if (html_size + needed_space + 1 > html_capacity) {
            // 一次性扩展更大的空间，减少后续realloc
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
        
        // 直接写入缓冲区，避免中间缓冲区
        if (isDir) {
            html_size += snprintf(html + html_size, html_capacity - html_size,
                "<tr><td class=\"name-cell\"><span class=\"icon folder-icon\">📁</span><span class=\"dir-link\" data-path=\"%s/\">%s/</span></td><td class=\"type\">&lt;dir&gt;</td></tr>\n",
                fileName, fileName);
        } else {
            LARGE_INTEGER fileSize;
            fileSize.LowPart = findData.nFileSizeLow;
            fileSize.HighPart = findData.nFileSizeHigh;
            html_size += snprintf(html + html_size, html_capacity - html_size,
                "<tr><td class=\"name-cell\"><span class=\"icon file-icon\">📄</span><span class=\"file-link\" data-path=\"%s\">%s</span></td><td class=\"size\">%s</td></tr>\n",
                fileName, fileName, format_file_size(fileSize.QuadPart));
        }
        
    } while (FindNextFileW(hFind, &findData));
    
    FindClose(hFind);
    
    const char* footer_and_script = 
        "                </tbody>\n"
        "            </table>\n"
        "        </div>\n"
        "        <div class=\"author\">\n"
        "            Created by <a href=\"https://github.com/Roy-Jin\" target=\"_blank\">GitHub/Roy-Jin</a>\n"
        "        </div>\n"
        "    </div>\n"
        "    <script>\n"
        "        (function() {\n"
        "            const tbody = document.querySelector('.directory-table tbody');\n"
        "            if (!tbody) return;\n"
        "            tbody.addEventListener('click', function(e) {\n"
        "                const row = e.target.closest('tr');\n"
        "                if (!row) return;\n"
        "                const linkElem = row.querySelector('[data-path]');\n"
        "                if (!linkElem) return;\n"
        "                const path = linkElem.getAttribute('data-path');\n"
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