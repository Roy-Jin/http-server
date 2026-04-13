#include "config.h"
#include "server.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

// 全局配置实例
Config g_config = {
    .port = DEFAULT_PORT,
    .root_dir = ".",
    .directory_listing = true,
    .max_threads = MAX_THREADS,
    .max_queue = MAX_QUEUE,
    .cors = true,
    .gzip = true,
    .index_files = {{"index.html"}, {"index.htm"}, {"index.php"}, {"default.html"}, {"default.htm"}}
};

// 显示当前配置
void config_show() {
    printf("\n" HEADER_COLOR "Starting up " SERVER_NAME ", serving %s" COLOR_RESET "\n\n", g_config.root_dir);
    printf(INFO_COLOR SERVER_NAME " version: " SERVER_VERSION "\n\n" COLOR_RESET);
    printf(HEADER_COLOR SERVER_NAME " settings:" COLOR_RESET "\n");
    printf(INFO_COLOR "GZip: " VALUE_COLOR "%s\n" COLOR_RESET, g_config.gzip ? "enabled" : "disabled");
    printf(INFO_COLOR "CORS: " VALUE_COLOR "%s\n" COLOR_RESET, g_config.cors ? "enabled" : "disabled");
    printf(INFO_COLOR "Cache: " VALUE_COLOR "3600 seconds\n" COLOR_RESET);
    printf(INFO_COLOR "Connection Timeout: " VALUE_COLOR "120 seconds\n" COLOR_RESET);
    printf(INFO_COLOR "Directory Listings: " VALUE_COLOR "%s\n" COLOR_RESET, g_config.directory_listing ? "visible" : "hidden");
    printf(INFO_COLOR "Max Threads: " VALUE_COLOR "%d\n" COLOR_RESET, g_config.max_threads);
    printf(INFO_COLOR "Max Queue: " VALUE_COLOR "%d\n\n" COLOR_RESET, g_config.max_queue);
}

// 配置初始化
bool config_init(int argc, char* argv[]) {
    // 解析命令行参数
    for (int i = 1; i < argc; i++) {
        if ((strcmp(argv[i], "--port") == 0 || strcmp(argv[i], "-p") == 0) && i + 1 < argc) {
            g_config.port = atoi(argv[i + 1]);
            if (g_config.port <= 0 || g_config.port > 65535) {
                g_config.port = DEFAULT_PORT;
            }
            i++;
        } else if ((strcmp(argv[i], "--root") == 0 || strcmp(argv[i], "-r") == 0) && i + 1 < argc) {
            strncpy(g_config.root_dir, argv[i + 1], sizeof(g_config.root_dir) - 1);
            g_config.root_dir[sizeof(g_config.root_dir) - 1] = '\0';
            i++;
        } else if (strcmp(argv[i], "--no-directory-listing") == 0 || strcmp(argv[i], "-d") == 0) {
            g_config.directory_listing = false;
        } else if ((strcmp(argv[i], "--max-threads") == 0 || strcmp(argv[i], "-t") == 0) && i + 1 < argc) {
            g_config.max_threads = atoi(argv[i + 1]);
            if (g_config.max_threads <= 0) {
                g_config.max_threads = MAX_THREADS;
            }
            i++;
        } else if ((strcmp(argv[i], "--max-queue") == 0 || strcmp(argv[i], "-q") == 0) && i + 1 < argc) {
            g_config.max_queue = atoi(argv[i + 1]);
            if (g_config.max_queue <= 0) {
                g_config.max_queue = MAX_QUEUE;
            }
            i++;
        } else if (strcmp(argv[i], "--no-cors") == 0 || strcmp(argv[i], "-c") == 0) {
            g_config.cors = false;
        } else if (strcmp(argv[i], "--no-gzip") == 0 || strcmp(argv[i], "-g") == 0) {
            g_config.gzip = false;
        } else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            printf(INFO_COLOR SERVER_NAME " version: " SERVER_VERSION "\n\n" COLOR_RESET);
            printf("Usage: http-server [options]\n");
            printf("Options:\n");
            printf("  -p, --port <port>          Set server port (default: %d)\n", DEFAULT_PORT);
            printf("  -r, --root <directory>     Set root directory (default: %s)\n", g_config.root_dir);
            printf("  -d, --no-directory-listing Disable directory listing (default: %s)\n", g_config.directory_listing ? "enabled" : "disabled");
            printf("  -t, --max-threads <n>      Set maximum number of threads (default: %d)\n", g_config.max_threads);
            printf("  -q, --max-queue <n>        Set maximum queue length (default: %d)\n", g_config.max_queue);
            printf("  -c, --no-cors              Disable CORS (default: %s)\n", g_config.cors ? "enabled" : "disabled");
            printf("  -g, --no-gzip              Disable GZip compression (default: %s)\n", g_config.gzip ? "enabled" : "disabled");
            printf("  -h, --help,                Show this help message\n");
            printf("  -v, --version,             Show version information\n");
            return false;
        } else if (strcmp(argv[i], "--version") == 0 || strcmp(argv[i], "-v") == 0) {
            printf(INFO_COLOR SERVER_NAME " version: " SERVER_VERSION "\n" COLOR_RESET);
            return false;
        } else if (atoi(argv[i]) > 0) {
            // 兼容旧的端口参数格式
            g_config.port = atoi(argv[i]);
            if (g_config.port <= 0 || g_config.port > 65535) {
                g_config.port = DEFAULT_PORT;
            }
        }
    }
    
    return true;
}