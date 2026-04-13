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
- ✅ **Thread pool support** - Uses thread pool to handle concurrent requests, improving performance
- ✅ **UTF-8 file path support** - Supports file paths with non-ASCII characters
- ✅ **CORS support** - Enabled by default, can be disabled
- ✅ **GZip compression support** - Enabled by default, can be disabled
- ✅ **File caching** - LRU cache strategy, max 100 items, max 10MB cache space
- ✅ **Range request support** - Supports HTTP Range requests for video playback and resume downloads
- ✅ **Index file support** - Auto-detects index.html, index.htm, index.php, default.html, default.htm
- ✅ **Cache control** - Default cache time 3600 seconds
- ✅ **Connection timeout** - Default connection timeout 120 seconds
- ✅ **Colored terminal output** - Supports colored log output for easier debugging

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
gcc -o http-server.exe main.c src\server.c src\http_handler.c src\http_response.c src\file_handler.c src\utils.c src\config.c -lws2_32 -liphlpapi -lz
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

1. Run the server: `./http-server.exe`
2. Access in browser: `http://localhost`

### Command Line Parameters

```bash
# Use default port (80)
.\http-server.exe

# Use specified port
.\http-server.exe 8080
.\http-server.exe 3000

# Specify root directory
.\http-server.exe --root D:\www
.\http-server.exe -r D:\www

# Disable directory listing
.\http-server.exe --no-directory-listing
.\http-server.exe -d

# Set maximum threads
.\http-server.exe --max-threads 20
.\http-server.exe -t 20

# Set maximum queue length
.\http-server.exe --max-queue 100
.\http-server.exe -q 100

# Enable CORS (enabled by default)
.\http-server.exe --cors

# Disable CORS
.\http-server.exe --no-cors
.\http-server.exe -c

# Disable GZip compression (enabled by default)
.\http-server.exe --no-gzip
.\http-server.exe -g

# Show help
.\http-server.exe --help
.\http-server.exe -h

# Show version information
.\http-server.exe --version
.\http-server.exe -v
```

### Full Command Line Options

| Option | Description | Default |
|--------|-------------|---------|
| `-p, --port <port>` | Set server port | 80 |
| `-r, --root <directory>` | Set root directory | . |
| `-d, --no-directory-listing` | Disable directory listing | Enabled |
| `-t, --max-threads <n>` | Set maximum number of threads | 10 |
| `-q, --max-queue <n>` | Set maximum queue length | 50 |
| `-c, --no-cors` | Disable CORS | Enabled |
| `-g, --no-gzip` | Disable GZip compression | Enabled |
| `-h, --help` | Show this help message | - |
| `-v, --version` | Show version information | - |

### Supported HTTP Methods

- `GET` - Get file or directory listing

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
├── README_zh.md              # Chinese project documentation
├── .gitignore                # Git ignore file
├── test/                     # Test directory
│   ├── www/                  # Test website files
│   │   ├── 0.png             # Test image
│   │   └── index.html        # Test home page
│   └── 0.txt                 # Test text file
└── src/                      # Source code directory
    ├── config.h              # Configuration header file
    ├── config.c              # Configuration implementation
    ├── server.h              # Server configuration and constants
    ├── server.c              # Server core logic
    ├── http_handler.h        # HTTP request handling
    ├── http_handler.c        # HTTP request handling implementation
    ├── http_response.h       # HTTP response generation
    ├── http_response.c       # HTTP response implementation
    ├── file_handler.h        # File handling
    ├── file_handler.c        # File handling implementation
    ├── utils.h               # Utility functions
    └── utils.c               # Utility functions implementation
```

## Code Module Description

### config.h / config.c
- Global configuration management
- Command line parameter parsing
- Configuration default value setting
- Index file configuration (index.html, index.htm, index.php, default.html, default.htm)

### server.h / server.c
- Server configuration and constant definitions
- Socket creation and server running logic
- Thread pool management and concurrent processing
- Default port: 80, buffer size: 8192 bytes
- Default max threads: 10, max queue length: 50
- Cache time: 3600 seconds, connection timeout: 120 seconds

### http_handler.h / http_handler.c
- HTTP request parsing and handling
- Request method validation and routing
- UTF-8 file path handling

### http_response.h / http_response.c
- HTTP response header generation
- Status code handling (200, 404, etc.)
- Redirect functionality

### file_handler.h / file_handler.c
- File content sending
- Directory listing generation
- File operation handling
- File cache management (LRU strategy)
- Range request support
- Default cache config: max 100 items, max 10MB

### utils.h / utils.c
- MIME type detection
- Local IP address retrieval
- Directory checking
- File size formatting
- URL decoding functionality
- Timestamp generation
- Colored terminal output

## License

This project is released under the [MIT License](LICENSE).

## Contributing

Welcome to submit Issues and Pull Requests to improve this project.

## Known Limitations

- Only supports Windows platform
- Does not support HTTPS/SSL encryption
- Does not support HTTP/2 protocol
- Does not support WebSocket protocol
- Does not support HTTP basic authentication

---

**Note**: This server is mainly for local development and testing, not recommended for production use.