#include "utils.h"
#include <stdio.h>
#include <string.h>
#include <iphlpapi.h>

#pragma comment(lib, "iphlpapi.lib")

const char *get_mime_type(const char *path)
{
    const char *dot = strrchr(path, '.');

    if (strcmp(dot, ".html") == 0 || strcmp(dot, ".htm") == 0)
        return "text/html; charset=utf-8";
    if (strcmp(dot, ".css") == 0)
        return "text/css; charset=utf-8";
    if (strcmp(dot, ".js") == 0)
        return "application/javascript; charset=utf-8";
    if (strcmp(dot, ".json") == 0)
        return "application/json; charset=utf-8";
    if (strcmp(dot, ".xml") == 0)
        return "application/xml; charset=utf-8";
    if (strcmp(dot, ".pdf") == 0)
        return "application/pdf";
    if (strcmp(dot, ".doc") == 0)
        return "application/msword";
    if (strcmp(dot, ".docx") == 0)
        return "application/vnd.openxmlformats-officedocument.wordprocessingml.document";
    if (strcmp(dot, ".xls") == 0)
        return "application/vnd.ms-excel";
    if (strcmp(dot, ".xlsx") == 0)
        return "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet";
    if (strcmp(dot, ".ppt") == 0)
        return "application/vnd.ms-powerpoint";
    if (strcmp(dot, ".pptx") == 0)
        return "application/vnd.openxmlformats-officedocument.presentationml.presentation";
    if (strcmp(dot, ".png") == 0)
        return "image/png";
    if (strcmp(dot, ".jpg") == 0 || strcmp(dot, ".jpeg") == 0)
        return "image/jpeg";
    if (strcmp(dot, ".gif") == 0)
        return "image/gif";
    if (strcmp(dot, ".svg") == 0)
        return "image/svg+xml";
    if (strcmp(dot, ".ico") == 0)
        return "image/x-icon";
    if (strcmp(dot, ".webp") == 0)
        return "image/webp";
    if (strcmp(dot, ".bmp") == 0)
        return "image/bmp";
    if (strcmp(dot, ".txt") == 0)
        return "text/plain; charset=utf-8";
    if (strcmp(dot, ".md") == 0)
        return "text/markdown; charset=utf-8";
    if (strcmp(dot, ".csv") == 0)
        return "text/csv; charset=utf-8";
    if (strcmp(dot, ".rtf") == 0)
        return "application/rtf";
    if (strcmp(dot, ".mp3") == 0)
        return "audio/mpeg";
    if (strcmp(dot, ".wav") == 0)
        return "audio/wav";
    if (strcmp(dot, ".ogg") == 0)
        return "audio/ogg";
    if (strcmp(dot, ".mp4") == 0)
        return "video/mp4";
    if (strcmp(dot, ".webm") == 0)
        return "video/webm";
    if (strcmp(dot, ".avi") == 0)
        return "video/avi";
    if (strcmp(dot, ".mov") == 0)
        return "video/quicktime";
    if (strcmp(dot, ".zip") == 0)
        return "application/zip";
    if (strcmp(dot, ".rar") == 0)
        return "application/x-rar-compressed";
    if (strcmp(dot, ".7z") == 0)
        return "application/x-7z-compressed";
    if (strcmp(dot, ".tar") == 0)
        return "application/x-tar";
    if (strcmp(dot, ".gz") == 0)
        return "application/gzip";
    if (strcmp(dot, ".exe") == 0)
        return "application/x-msdownload";
    if (strcmp(dot, ".dll") == 0)
        return "application/x-msdownload";
    if (strcmp(dot, ".iso") == 0)
        return "application/x-iso9660-image";
    if (strcmp(dot, ".apk") == 0)
        return "application/vnd.android.package-archive";
    if (strcmp(dot, ".ipa") == 0)
        return "application/octet-stream";
    if (strcmp(dot, ".woff") == 0)
        return "font/woff";
    if (strcmp(dot, ".woff2") == 0)
        return "font/woff2";
    if (strcmp(dot, ".ttf") == 0)
        return "font/ttf";
    if (strcmp(dot, ".otf") == 0)
        return "font/otf";

    // 添加更多文本类型文件的支持，优先预览
    if (strcmp(dot, ".c") == 0)
        return "text/plain; charset=utf-8";
    if (strcmp(dot, ".h") == 0)
        return "text/plain; charset=utf-8";
    if (strcmp(dot, ".cpp") == 0)
        return "text/plain; charset=utf-8";
    if (strcmp(dot, ".hpp") == 0)
        return "text/plain; charset=utf-8";
    if (strcmp(dot, ".java") == 0)
        return "text/plain; charset=utf-8";
    if (strcmp(dot, ".py") == 0)
        return "text/plain; charset=utf-8";
    if (strcmp(dot, ".ts") == 0)
        return "text/plain; charset=utf-8";
    if (strcmp(dot, ".php") == 0)
        return "text/plain; charset=utf-8";
    if (strcmp(dot, ".bat") == 0)
        return "text/plain; charset=utf-8";
    if (strcmp(dot, ".sh") == 0)
        return "text/plain; charset=utf-8";
    if (strcmp(dot, ".cmd") == 0)
        return "text/plain; charset=utf-8";
    if (strcmp(dot, ".ps1") == 0)
        return "text/plain; charset=utf-8";
    if (strcmp(dot, ".sql") == 0)
        return "text/plain; charset=utf-8";
    if (strcmp(dot, ".yaml") == 0 || strcmp(dot, ".yml") == 0)
        return "text/plain; charset=utf-8";
    if (strcmp(dot, ".log") == 0)
        return "text/plain; charset=utf-8";
    if (strcmp(dot, ".ini") == 0)
        return "text/plain; charset=utf-8";
    if (strcmp(dot, ".conf") == 0)
        return "text/plain; charset=utf-8";
    if (strcmp(dot, ".config") == 0)
        return "text/plain; charset=utf-8";
    if (strcmp(dot, ".env") == 0)
        return "text/plain; charset=utf-8";
    if (strcmp(dot, ".gitignore") == 0)
        return "text/plain; charset=utf-8";
    if (strcmp(dot, ".dockerignore") == 0)
        return "text/plain; charset=utf-8";
    if (strcmp(dot, ".editorconfig") == 0)
        return "text/plain; charset=utf-8";

    return "text/plain; charset=utf-8";
}

void print_local_ips(void)
{
    ULONG outBufLen = 15000;
    IP_ADAPTER_ADDRESSES *adapterAddresses = (IP_ADAPTER_ADDRESSES *)malloc(outBufLen);

    if (adapterAddresses == NULL)
    {
        printf("Error: Memory allocation failed\n");
        return;
    }

    DWORD dwRetVal = GetAdaptersAddresses(AF_INET, GAA_FLAG_INCLUDE_PREFIX | GAA_FLAG_SKIP_MULTICAST, NULL, adapterAddresses, &outBufLen);

    if (dwRetVal == NO_ERROR)
    {
        printf("==========================================\n");
        printf("  HTTP Server Started\n");
        printf("  Local IP Addresses:\n");

        IP_ADAPTER_ADDRESSES *adapter = adapterAddresses;
        int ipCount = 0;

        while (adapter)
        {
            if (adapter->OperStatus == IfOperStatusUp)
            {
                IP_ADAPTER_UNICAST_ADDRESS *unicast = adapter->FirstUnicastAddress;
                while (unicast)
                {
                    if (unicast->Address.lpSockaddr->sa_family == AF_INET)
                    {
                        char ipStr[INET_ADDRSTRLEN];
                        struct sockaddr_in *sa = (struct sockaddr_in *)(unicast->Address.lpSockaddr);

                        inet_ntop(AF_INET, &(sa->sin_addr), ipStr, INET_ADDRSTRLEN);

                        if (strcmp(ipStr, "127.0.0.1") != 0)
                        {
                            printf("   - %s\n", ipStr);
                            ipCount++;
                        }
                    }
                    unicast = unicast->Next;
                }
            }
            adapter = adapter->Next;
        }

        if (ipCount == 0)
        {
            printf("   - (No active network interfaces found)\n");
        }
        printf("==========================================\n");
    }
    else
    {
        printf("Error: GetAdaptersAddresses failed with error: %d\n", dwRetVal);
    }

    free(adapterAddresses);
}

int is_directory(const char *path)
{
    wchar_t wpath[PATH_MAX_LEN];
    MultiByteToWideChar(CP_UTF8, 0, path, -1, wpath, PATH_MAX_LEN);
    DWORD attrib = GetFileAttributesW(wpath);
    if (attrib == INVALID_FILE_ATTRIBUTES)
        return 0;
    return (attrib & FILE_ATTRIBUTE_DIRECTORY) != 0;
}

const char *format_file_size(long long bytes)
{
    static char buffer[32];

    if (bytes < 1024)
    {
        snprintf(buffer, sizeof(buffer), "%lld bytes", bytes);
    }
    else if (bytes < 1024 * 1024)
    {
        double kb = bytes / 1024.0;
        snprintf(buffer, sizeof(buffer), "%.1f KB", kb);
    }
    else if (bytes < 1024 * 1024 * 1024)
    {
        double mb = bytes / (1024.0 * 1024.0);
        snprintf(buffer, sizeof(buffer), "%.1f MB", mb);
    }
    else
    {
        double gb = bytes / (1024.0 * 1024.0 * 1024.0);
        snprintf(buffer, sizeof(buffer), "%.1f GB", gb);
    }

    return buffer;
}

void url_decode(const char *encoded, char *decoded, size_t max_len)
{
    size_t i = 0, j = 0;
    while (encoded[i] != '\0' && j < max_len - 1)
    {
        if (encoded[i] == '%' && encoded[i + 1] != '\0' && encoded[i + 2] != '\0')
        {
            // 处理URL编码的字符，如%20
            char hex[3] = {encoded[i + 1], encoded[i + 2], '\0'};
            int val = strtol(hex, NULL, 16);
            decoded[j++] = (char)val;
            i += 3;
        }
        else
        {
            // 普通字符直接复制
            decoded[j++] = encoded[i++];
        }
    }
    decoded[j] = '\0';
}