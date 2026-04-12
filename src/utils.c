#include "utils.h"
#include <stdio.h>
#include <string.h>
#include <iphlpapi.h>

#pragma comment(lib, "iphlpapi.lib")

const char* get_mime_type(const char* path) {
    const char* dot = strrchr(path, '.');
    if (!dot) return "application/octet-stream";

    if (strcmp(dot, ".html") == 0 || strcmp(dot, ".htm") == 0) return "text/html; charset=utf-8";
    if (strcmp(dot, ".css") == 0) return "text/css";
    if (strcmp(dot, ".js") == 0) return "application/javascript";
    if (strcmp(dot, ".json") == 0) return "application/json";
    if (strcmp(dot, ".png") == 0) return "image/png";
    if (strcmp(dot, ".jpg") == 0 || strcmp(dot, ".jpeg") == 0) return "image/jpeg";
    if (strcmp(dot, ".gif") == 0) return "image/gif";
    if (strcmp(dot, ".svg") == 0) return "image/svg+xml";
    if (strcmp(dot, ".ico") == 0) return "image/x-icon";
    if (strcmp(dot, ".txt") == 0) return "text/plain";
    
    return "application/octet-stream";
}

void print_local_ips(void) {
    ULONG outBufLen = 15000;
    IP_ADAPTER_ADDRESSES* adapterAddresses = (IP_ADAPTER_ADDRESSES*)malloc(outBufLen);
    
    if (adapterAddresses == NULL) {
        printf("Error: Memory allocation failed\n");
        return;
    }

    DWORD dwRetVal = GetAdaptersAddresses(AF_INET, GAA_FLAG_INCLUDE_PREFIX | GAA_FLAG_SKIP_MULTICAST, 
                                          NULL, adapterAddresses, &outBufLen);

    if (dwRetVal == NO_ERROR) {
        printf("==========================================\n");
        printf("  HTTP Server Started\n");
        printf("  Local IP Addresses:\n");
        
        IP_ADAPTER_ADDRESSES* adapter = adapterAddresses;
        int ipCount = 0;

        while (adapter) {
            if (adapter->OperStatus == IfOperStatusUp) {
                IP_ADAPTER_UNICAST_ADDRESS* unicast = adapter->FirstUnicastAddress;
                while (unicast) {
                    if (unicast->Address.lpSockaddr->sa_family == AF_INET) {
                        char ipStr[INET_ADDRSTRLEN];
                        struct sockaddr_in* sa = (struct sockaddr_in*)(unicast->Address.lpSockaddr);
                        
                        inet_ntop(AF_INET, &(sa->sin_addr), ipStr, INET_ADDRSTRLEN);
                        
                        if (strcmp(ipStr, "127.0.0.1") != 0) {
                            printf("   - %s\n", ipStr);
                            ipCount++;
                        }
                    }
                    unicast = unicast->Next;
                }
            }
            adapter = adapter->Next;
        }

        if (ipCount == 0) {
            printf("   - (No active network interfaces found)\n");
        }
        printf("==========================================\n");
    } else {
        printf("Error: GetAdaptersAddresses failed with error: %d\n", dwRetVal);
    }

    free(adapterAddresses);
}

int is_directory(const char* path) {
    DWORD attrib = GetFileAttributesA(path);
    if (attrib == INVALID_FILE_ATTRIBUTES) return 0;
    return (attrib & FILE_ATTRIBUTE_DIRECTORY) != 0;
}

const char* format_file_size(long long bytes) {
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