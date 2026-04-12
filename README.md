# HTTP Server

A lightweight HTTP server written in pure C, supporting Windows platform. This project aims to provide a simple and efficient file server suitable for local development and testing.

[English](README.md) | [中文](README_zh.md)

## Features

- ✅ **Static file serving** - Supports HTML, CSS, JavaScript, images and other file types
- ✅ **Directory listing** - Automatically generates directory index pages
- ✅ **MIME type detection** - Automatically sets correct Content-Type based on file extension
- ✅ **File size formatting** - Automatically converts file size to human-readable format (KB/MB/GB)
- ✅ **Custom port** - Supports command line parameter to specify listening port
- ✅ **Local IP display** - Shows all available local IPv4 addresses on startup
- ✅ **HTTP status codes** - Supports standard HTTP status codes like 200, 404
- ✅ **Redirect support** - Supports HTTP redirect functionality

## System Requirements

- **Operating System**: Windows Vista or higher
- **Compiler**: GCC (MinGW) or compatible C compiler
- **Dependencies**: Windows Socket 2.0 (ws2_32), IP Helper API (iphlpapi)

## Quick Start

### 1. Build the project

Use the provided build script:

```bash
.\build.bat
```

Or compile manually:

```bash
gcc -o http-server.exe main.c src\server.c src\http_handler.c src\http_response.c src\file_handler.c src\utils.c -lws2_32 -liphlpapi
```

### 2. Run the server

Default port (80):

```bash
.\http-server.exe
```

Specify port (e.g., 8080):

```bash
.\http-server.exe 8080
```

### 3. Access the server

After the server starts, it will display available local IP addresses. Access in your browser:

```
http://localhost:80
```

Or use the displayed IP address:

```
http://192.168.x.x:80
```

## Usage

### Basic Usage

1. Run the server: `.\http-server.exe`
2. Access in browser: `http://localhost`

### Command Line Parameters

```bash
# Use default port (80)
.\http-server.exe

# Use specified port
.\http-server.exe 8080
.\http-server.exe 3000
```

### Supported HTTP Methods

- `GET` - Get file or directory listing
- `HEAD` - Get response header information

### Supported File Types

The server supports common file types:

- Text files: `.html`, `.css`, `.js`, `.txt`, `.json`, `.xml`
- Image files: `.png`, `.jpg`, `.jpeg`, `.gif`, `.bmp`, `.ico`
- Others: `.pdf`, `.zip`, `.mp3`, `.mp4`

## Project Structure

```
http-server/
├── main.c                    # Program entry point
├── build.bat                 # Build script
├── LICENSE                   # MIT License
├── README.md                 # Project documentation
├── .gitignore               # Git ignore file
└── src/                      # Source code directory
    ├── server.h             # Server configuration and constants
    ├── server.c             # Server core logic
    ├── http_handler.h       # HTTP request handling
    ├── http_handler.c       # HTTP request handling implementation
    ├── http_response.h      # HTTP response generation
    ├── http_response.c      # HTTP response implementation
    ├── file_handler.h       # File handling
    ├── file_handler.c       # File handling implementation
    ├── utils.h              # Utility functions
    └── utils.c              # Utility functions implementation
```

## Code Module Description

### server.h / server.c
- Server configuration and constant definitions
- Socket creation and server running logic
- Default port: 80, buffer size: 8192 bytes

### http_handler.h / http_handler.c
- HTTP request parsing and handling
- Request method validation and routing

### http_response.h / http_response.c
- HTTP response header generation
- Status code handling (200, 404, etc.)
- Redirect functionality

### file_handler.h / file_handler.c
- File content sending
- Directory listing generation
- File operation handling

### utils.h / utils.c
- MIME type detection
- Local IP address retrieval
- Directory checking
- File size formatting

## License

This project is released under the [MIT License](LICENSE).

## Contributing

Welcome to submit Issues and Pull Requests to improve this project.

## Known Limitations

- Only supports Windows platform
- Does not support HTTPS
- Does not support HTTP/2
- Limited concurrent connection handling capability

---

**Note**: This server is mainly for local development and testing, not recommended for production use.