# HTTP 服务器

一个用纯C语言编写的轻量级HTTP服务器，支持Windows平台。该项目旨在提供一个简单、高效的文件服务器，适用于本地开发和测试。

[English](README.md) | [中文](README_zh.md)

## 功能特性

- ✅ **静态文件服务** - 支持HTML、CSS、JavaScript、图片等文件类型
- ✅ **目录列表** - 自动生成目录索引页面
- ✅ **MIME类型检测** - 根据文件扩展名自动设置正确的Content-Type
- ✅ **文件大小格式化** - 自动将文件大小转换为人类可读格式（KB/MB/GB）
- ✅ **自定义端口** - 支持命令行参数指定监听端口
- ✅ **本地IP显示** - 启动时显示所有可用的本地IPv4地址
- ✅ **HTTP状态码** - 支持200、404等标准HTTP状态码
- ✅ **重定向支持** - 支持HTTP重定向功能
- ✅ **线程池支持** - 使用线程池处理并发请求，提高性能
- ✅ **UTF-8文件路径支持** - 支持包含非ASCII字符的文件路径
- ✅ **CORS支持** - 默认启用跨域资源共享，可选择禁用
- ✅ **GZip压缩支持** - 默认启用GZip压缩，可选择禁用
- ✅ **文件缓存** - LRU缓存策略，最大100项，最大10MB缓存空间
- ✅ **范围请求支持** - 支持HTTP Range请求，可用于视频播放和断点续传
- ✅ **索引文件支持** - 自动识别 index.html, index.htm, index.php, default.html, default.htm
- ✅ **缓存控制** - 默认缓存时间3600秒
- ✅ **连接超时** - 默认连接超时120秒
- ✅ **彩色终端输出** - 支持彩色日志输出，便于调试

## 系统要求

- **操作系统**: Windows Vista 或更高版本
- **编译器**: GCC (MinGW) 或兼容的C编译器
- **依赖库**: Windows Socket 2.0 (ws2_32), IP Helper API (iphlpapi)

## 快速开始

### 1. 构建项目

使用提供的构建脚本：

```bash
.\build.bat
```

或者手动编译：

```bash
gcc -o http-server.exe main.c src\server.c src\http_handler.c src\http_response.c src\file_handler.c src\utils.c src\config.c -lws2_32 -liphlpapi -lz
```

### 2. 运行服务器

默认端口（80）：

```bash
.\http-server.exe
```

指定端口（例如8080）：

```bash
.\http-server.exe 8080
```

### 3. 访问服务器

服务器启动后，会显示可用的本地IP地址。在浏览器中访问：

```
http://localhost:80
```

或使用显示的IP地址：

```
http://192.168.x.x:80
```

## 使用方法

### 基本使用

1. 运行服务器：`./http-server.exe`
2. 在浏览器中访问 `http://localhost`

### 命令行参数

```bash
# 使用默认端口（80）
.\http-server.exe

# 使用指定端口
.\http-server.exe 8080
.\http-server.exe 3000

# 指定根目录
.\http-server.exe --root D:\www
.\http-server.exe -r D:\www

# 禁用目录列表
.\http-server.exe --no-directory-listing
.\http-server.exe -d

# 设置最大线程数
.\http-server.exe --max-threads 20
.\http-server.exe -t 20

# 设置最大队列长度
.\http-server.exe --max-queue 100
.\http-server.exe -q 100

# 启用CORS（默认已启用）
.\http-server.exe --cors

# 禁用CORS
.\http-server.exe --no-cors
.\http-server.exe -c

# 禁用GZip压缩（默认已启用）
.\http-server.exe --no-gzip
.\http-server.exe -g

# 显示帮助
.\http-server.exe --help
.\http-server.exe -h

# 显示版本信息
.\http-server.exe --version
.\http-server.exe -v
```

### 完整命令行选项

| 选项 | 描述 | 默认值 |
|------|------|--------|
| `--port, -p <port>` | 设置服务器端口 | 80 |
| `--root, -r <directory>` | 设置根目录 | . |
| `--no-directory-listing, -d` | 禁用目录列表 | 启用 |
| `--max-threads, -t <n>` | 设置最大线程数 | 10 |
| `--max-queue, -q <n>` | 设置最大队列长度 | 50 |
| `--no-cors, -c` | 禁用CORS | 启用 |
| `--no-gzip, -g` | 禁用GZip压缩 | 启用 |
| `--help, -h` | 显示帮助信息 | - |
| `--version, -v` | 显示版本信息 | - |

### 支持的HTTP方法

- `GET` - 获取文件或目录列表

### 文件类型支持

服务器支持常见的文件类型：

- 文本文件：`.html`, `.css`, `.js`, `.txt`, `.json`, `.xml`
- 图片文件：`.png`, `.jpg`, `.jpeg`, `.gif`, `.bmp`, `.ico`
- 其他：`.pdf`, `.zip`, `.mp3`, `.mp4`

## 项目结构

```
http-server/
├── main.c                    # 程序入口点
├── build.bat                 # 构建脚本
├── LICENSE                   # MIT许可证
├── README.md                 # 项目文档
├── README_zh.md              # 中文项目文档
├── .gitignore                # Git忽略文件
├── test/                     # 测试目录
│   ├── www/                  # 测试网站文件
│   │   ├── 0.png             # 测试图片
│   │   └── index.html        # 测试首页
│   └── 0.txt                 # 测试文本文件
└── src/                      # 源代码目录
    ├── config.h              # 配置头文件
    ├── config.c              # 配置实现
    ├── server.h              # 服务器配置和常量
    ├── server.c              # 服务器核心逻辑
    ├── http_handler.h        # HTTP请求处理
    ├── http_handler.c        # HTTP请求处理实现
    ├── http_response.h       # HTTP响应生成
    ├── http_response.c       # HTTP响应实现
    ├── file_handler.h        # 文件处理
    ├── file_handler.c        # 文件处理实现
    ├── utils.h               # 工具函数
    └── utils.c               # 工具函数实现
```

## 代码模块说明

### config.h / config.c
- 全局配置管理
- 命令行参数解析
- 配置默认值设置
- 索引文件配置（index.html, index.htm, index.php, default.html, default.htm）

### server.h / server.c
- 服务器配置和常量定义
- 套接字创建和服务器运行逻辑
- 线程池管理和并发处理
- 默认端口：80，缓冲区大小：8192字节
- 默认最大线程数：10，最大队列长度：50
- 缓存时间：3600秒，连接超时：120秒

### http_handler.h / http_handler.c
- HTTP请求解析和处理
- 请求方法验证和路由
- UTF-8文件路径处理

### http_response.h / http_response.c
- HTTP响应头生成
- 状态码处理（200, 404等）
- 重定向功能

### file_handler.h / file_handler.c
- 文件内容发送
- 目录列表生成
- 文件操作处理
- 文件缓存管理（LRU策略）
- 范围请求支持（Range requests）
- 默认缓存配置：最大100项，最大10MB

### utils.h / utils.c
- MIME类型检测
- 本地IP地址获取
- 目录检查
- 文件大小格式化
- URL解码功能
- 时间戳生成
- 彩色终端输出

## 许可证

本项目基于 [MIT License](LICENSE) 发布。

## 贡献

欢迎提交Issue和Pull Request来改进这个项目。

## 已知限制

- 仅支持Windows平台
- 不支持HTTPS/SSL加密
- 不支持HTTP/2协议
- 不支持WebSocket协议
- 不支持HTTP基本认证

---

**提示**: 此服务器主要用于本地开发和测试，不建议在生产环境中使用。