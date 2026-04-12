#include "file_handler.h"
#include "http_response.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void send_file_content(SOCKET client, FILE *file, const char *mime_type, long file_size)
{
    send_header(client, 200, "OK", mime_type, file_size);

    char buffer[BUFFER_SIZE];
    size_t bytes_read;
    while ((bytes_read = fread(buffer, 1, BUFFER_SIZE, file)) > 0)
    {
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
        "        .dir-link, .file-link, .parent-link {\n"
        "            color: #4285f4;\n"
        "            text-decoration: none;\n"
        "            font-weight: 500;\n"
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
    
    send_header(client, 200, "OK", "text/html; charset=utf-8", (long)html_size);
    send(client, html, (int)html_size, 0);
    
    free(html);
}