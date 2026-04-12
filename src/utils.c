#include "utils.h"
#include "config.h"
#include <stdio.h>
#include <string.h>
#include <iphlpapi.h>
#include <time.h>

#pragma comment(lib, "iphlpapi.lib")

// MIME类型映射结构
typedef struct {
    const char* extension;
    const char* mime_type;
} MimeTypeMapping;

// MIME类型映射表
static const MimeTypeMapping mime_type_mappings[] = {
    // 文档类型
    {".html", "text/html; charset=utf-8"},
    {".htm", "text/html; charset=utf-8"},
    {".css", "text/css; charset=utf-8"},
    {".js", "application/javascript; charset=utf-8"},
    {".json", "application/json; charset=utf-8"},
    {".xml", "application/xml; charset=utf-8"},
    {".pdf", "application/pdf"},
    {".doc", "application/msword"},
    {".docx", "application/vnd.openxmlformats-officedocument.wordprocessingml.document"},
    {".xls", "application/vnd.ms-excel"},
    {".xlsx", "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet"},
    {".ppt", "application/vnd.ms-powerpoint"},
    {".pptx", "application/vnd.openxmlformats-officedocument.presentationml.presentation"},
    
    // 图片类型
    {".png", "image/png"},
    {".jpg", "image/jpeg"},
    {".jpeg", "image/jpeg"},
    {".gif", "image/gif"},
    {".svg", "image/svg+xml"},
    {".ico", "image/x-icon"},
    {".webp", "image/webp"},
    {".bmp", "image/bmp"},
    
    // 文本类型
    {".txt", "text/plain; charset=utf-8"},
    {".md", "text/markdown; charset=utf-8"},
    {".csv", "text/csv; charset=utf-8"},
    {".rtf", "application/rtf"},
    
    // 音频类型
    {".mp3", "audio/mpeg"},
    {".wav", "audio/wav"},
    {".ogg", "audio/ogg"},
    
    // 视频类型
    {".mp4", "video/mp4"},
    {".webm", "video/webm"},
    {".avi", "video/avi"},
    {".mov", "video/quicktime"},
    
    // 压缩文件
    {".zip", "application/zip"},
    {".rar", "application/x-rar-compressed"},
    {".7z", "application/x-7z-compressed"},
    {".tar", "application/x-tar"},
    {".gz", "application/gzip"},
    
    // 可执行文件
    {".exe", "application/x-msdownload"},
    {".dll", "application/x-msdownload"},
    
    // 其他类型
    {".iso", "application/x-iso9660-image"},
    {".apk", "application/vnd.android.package-archive"},
    {".ipa", "application/octet-stream"},
    
    // 字体类型
    {".woff", "font/woff"},
    {".woff2", "font/woff2"},
    {".ttf", "font/ttf"},
    {".otf", "font/otf"},
    
    // 代码文件类型
    {".c", "text/plain; charset=utf-8"},
    {".h", "text/plain; charset=utf-8"},
    {".cpp", "text/plain; charset=utf-8"},
    {".hpp", "text/plain; charset=utf-8"},
    {".java", "text/plain; charset=utf-8"},
    {".py", "text/plain; charset=utf-8"},
    {".ts", "text/plain; charset=utf-8"},
    {".php", "text/plain; charset=utf-8"},
    {".bat", "text/plain; charset=utf-8"},
    {".sh", "text/plain; charset=utf-8"},
    {".cmd", "text/plain; charset=utf-8"},
    {".ps1", "text/plain; charset=utf-8"},
    {".sql", "text/plain; charset=utf-8"},
    {".yaml", "text/plain; charset=utf-8"},
    {".yml", "text/plain; charset=utf-8"},
    {".log", "text/plain; charset=utf-8"},
    {".ini", "text/plain; charset=utf-8"},
    {".conf", "text/plain; charset=utf-8"},
    {".config", "text/plain; charset=utf-8"},
    {".env", "text/plain; charset=utf-8"},
    {".gitignore", "text/plain; charset=utf-8"},
    {".dockerignore", "text/plain; charset=utf-8"},
    {".editorconfig", "text/plain; charset=utf-8"},
    
    // 终止符
    {NULL, NULL}
};

const char *get_mime_type(const char *path) {
    const char *dot = strrchr(path, '.');
    if (!dot) {
        return "text/plain; charset=utf-8";
    }
    
    // 查找MIME类型映射
    for (int i = 0; mime_type_mappings[i].extension != NULL; i++) {
        if (strcmp(dot, mime_type_mappings[i].extension) == 0) {
            return mime_type_mappings[i].mime_type;
        }
    }
    
    return "text/plain; charset=utf-8";
}

void print_local_ips(void) {
    ULONG outBufLen = 15000;
    IP_ADAPTER_ADDRESSES *adapterAddresses = (IP_ADAPTER_ADDRESSES *)malloc(outBufLen);

    if (adapterAddresses == NULL) {
        printf(ERROR_COLOR "Error: Memory allocation failed" COLOR_RESET "\n");
        return;
    }

    DWORD dwRetVal = GetAdaptersAddresses(AF_INET, GAA_FLAG_INCLUDE_PREFIX | GAA_FLAG_SKIP_MULTICAST, NULL, adapterAddresses, &outBufLen);

    if (dwRetVal == NO_ERROR) {
        printf(HEADER_COLOR "Available on:\n" COLOR_RESET);

        IP_ADAPTER_ADDRESSES *adapter = adapterAddresses;

        while (adapter) {
            if (adapter->OperStatus == IfOperStatusUp) {
                IP_ADAPTER_UNICAST_ADDRESS *unicast = adapter->FirstUnicastAddress;
                while (unicast) {
                    if (unicast->Address.lpSockaddr->sa_family == AF_INET) {
                        char ipStr[INET_ADDRSTRLEN];
                        struct sockaddr_in *sa = (struct sockaddr_in *)(unicast->Address.lpSockaddr);

                        inet_ntop(AF_INET, &(sa->sin_addr), ipStr, INET_ADDRSTRLEN);

                        if (strcmp(ipStr, "127.0.0.1") != 0) {
                            printf(INFO_COLOR "  http://%s:%d\n" COLOR_RESET, ipStr, g_config.port);
                        }
                    }
                    unicast = unicast->Next;
                }
            }
            adapter = adapter->Next;
        }

        printf(INFO_COLOR "  http://127.0.0.1:%d\n" COLOR_RESET, g_config.port);
    } else {
        printf(ERROR_COLOR "Error: GetAdaptersAddresses failed with error: %d" COLOR_RESET "\n", dwRetVal);
    }

    free(adapterAddresses);
}

int is_directory(const char *path) {
    wchar_t wpath[PATH_MAX_LEN];
    MultiByteToWideChar(CP_UTF8, 0, path, -1, wpath, PATH_MAX_LEN);
    DWORD attrib = GetFileAttributesW(wpath);
    if (attrib == INVALID_FILE_ATTRIBUTES)
        return 0;
    return (attrib & FILE_ATTRIBUTE_DIRECTORY) != 0;
}

const char *format_file_size(long long bytes) {
    static char buffer[32];

    if (bytes < 1024) {
        snprintf(buffer, sizeof(buffer), "%lld bytes", bytes);
    } else if (bytes < 1024 * 1024) {
        double kb = bytes / 1024.0;
        snprintf(buffer, sizeof(buffer), "%.1f KB", kb);
    } else if (bytes < 1024 * 1024 * 1024) {
        double mb = bytes / (1024.0 * 1024.0);
        snprintf(buffer, sizeof(buffer), "%.1f MB", mb);
    } else {
        double gb = bytes / (1024.0 * 1024.0 * 1024.0);
        snprintf(buffer, sizeof(buffer), "%.1f GB", gb);
    }

    return buffer;
}

void url_decode(const char *encoded, char *decoded, size_t max_len) {
    size_t i = 0, j = 0;
    while (encoded[i] != '\0' && j < max_len - 1) {
        if (encoded[i] == '%' && encoded[i + 1] != '\0' && encoded[i + 2] != '\0') {
            // 处理URL编码的字符，如%20
            char hex[3] = {encoded[i + 1], encoded[i + 2], '\0'};
            int val = strtol(hex, NULL, 16);
            decoded[j++] = (char)val;
            i += 3;
        } else {
            // 普通字符直接复制
            decoded[j++] = encoded[i++];
        }
    }
    decoded[j] = '\0';
}

// 获取当前时间戳字符串
const char* get_timestamp(void) {
    static char buffer[32];
    time_t now = time(NULL);
    struct tm* tm_info = localtime(&now);
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", tm_info);
    return buffer;
}