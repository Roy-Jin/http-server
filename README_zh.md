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
gcc -o http-server.exe main.c src\server.c src\http_handler.c src\http_response.c src\file_handler.c src\utils.c -lws2_32 -liphlpapi
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

1. 运行服务器：`.\http-server.exe`
2. 在浏览器中访问 `http://localhost`

### 命令行参数

```bash
# 使用默认端口（80）
.\http-server.exe

# 使用指定端口
.\http-server.exe 8080
.\http-server.exe 3000
```

### 支持的HTTP方法

- `GET` - 获取文件或目录列表
- `HEAD` - 获取响应头信息

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
├── .gitignore               # Git忽略文件
└── src/                      # 源代码目录
    ├── server.h             # 服务器配置和常量
    ├── server.c             # 服务器核心逻辑
    ├── http_handler.h       # HTTP请求处理
    ├── http_handler.c       # HTTP请求处理实现
    ├── http_response.h      # HTTP响应生成
    ├── http_response.c      # HTTP响应实现
    ├── file_handler.h       # 文件处理
    ├── file_handler.c       # 文件处理实现
    ├── utils.h              # 工具函数
    └── utils.c              # 工具函数实现
```

## 代码模块说明

### server.h / server.c
- 服务器配置和常量定义
- 套接字创建和服务器运行逻辑
- 默认端口：80，缓冲区大小：8192字节

### http_handler.h / http_handler.c
- HTTP请求解析和处理
- 请求方法验证和路由

### http_response.h / http_response.c
- HTTP响应头生成
- 状态码处理（200, 404等）
- 重定向功能

### file_handler.h / file_handler.c
- 文件内容发送
- 目录列表生成
- 文件操作处理

### utils.h / utils.c
- MIME类型检测
- 本地IP地址获取
- 目录检查
- 文件大小格式化

## 许可证

本项目基于 [MIT License](LICENSE) 发布。

## 贡献

欢迎提交Issue和Pull Request来改进这个项目。

## 已知限制

- 仅支持Windows平台
- 不支持HTTPS
- 不支持HTTP/2
- 并发连接处理能力有限

---

**提示**: 此服务器主要用于本地开发和测试，不建议在生产环境中使用。