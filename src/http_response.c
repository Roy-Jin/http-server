#include "http_response.h"
#include "config.h"
#include <stdio.h>
#include <string.h>
#include <time.h>

// 生成ETag
void generate_etag(char* etag, long content_length, time_t modified_time) {
    sprintf(etag, "\"%ld-%ld\"", content_length, modified_time);
}

void send_header(SOCKET client, int status_code, const char* status_text, 
                 const char* mime_type, long content_length, time_t modified_time) {
    char header[BUFFER_SIZE];
    char etag[64];
    generate_etag(etag, content_length, modified_time);
    
    int header_len = sprintf(header,
        "HTTP/1.1 %d %s\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %ld\r\n"
        "Connection: close\r\n"
        "Server: " SERVER_NAME "/" SERVER_VERSION "\r\n"
        "Cache-Control: max-age=3600\r\n"
        "ETag: %s\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "X-Content-Type-Options: nosniff\r\n"
        "X-Frame-Options: SAMEORIGIN\r\n"
        "X-XSS-Protection: 1; mode=block\r\n"
        "\r\n",
        status_code, status_text, mime_type, content_length, etag);
    
    send(client, header, header_len, 0);
}

void send_404(SOCKET client) {

    const char* body = 
        "<!DOCTYPE html>\n"
        "<html lang=\"en\">\n"
        "<head>\n"
        "    <meta charset=\"UTF-8\">\n"
        "    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n"
        "    <title>404 - Page Not Found</title>\n"
        "    <style>\n"
        "        body {\n"
        "            font-family: Arial, sans-serif;\n"
        "            background-color: #f8f9fa;\n"
        "            color: #333;\n"
        "            margin: 0;\n"
        "            padding: 0;\n"
        "            display: flex;\n"
        "            justify-content: center;\n"
        "            align-items: center;\n"
        "            min-height: 100vh;\n"
        "            text-align: center;\n"
        "        }\n"
        "        .container {\n"
        "            max-width: 600px;\n"
        "            padding: 40px 20px;\n"
        "        }\n"
        "        .error-code {\n"
        "            font-size: 72px;\n"
        "            font-weight: bold;\n"
        "            color: #dc3545;\n"
        "            margin-bottom: 20px;\n"
        "        }\n"
        "        .error-title {\n"
        "            font-size: 24px;\n"
        "            font-weight: bold;\n"
        "            margin-bottom: 20px;\n"
        "            color: #212529;\n"
        "        }\n"
        "        .error-message {\n"
        "            font-size: 16px;\n"
        "            line-height: 1.5;\n"
        "            margin-bottom: 30px;\n"
        "            color: #6c757d;\n"
        "        }\n"
        "        .home-button {\n"
        "            display: inline-block;\n"
        "            background-color: #007bff;\n"
        "            color: white;\n"
        "            padding: 12px 24px;\n"
        "            border-radius: 4px;\n"
        "            text-decoration: none;\n"
        "            font-weight: bold;\n"
        "            font-size: 16px;\n"
        "        }\n"
        "        .home-button:hover {\n"
        "            background-color: #0056b3;\n"
        "        }\n"
        "    </style>\n"
        "</head>\n"
        "<body>\n"
        "    <div class=\"container\">\n"
        "        <div class=\"error-code\">404</div>\n"
        "        <h1 class=\"error-title\">Page Not Found</h1>\n"
        "        <p class=\"error-message\">\n"
        "            The page you are looking for might have been removed, had its name changed,\n"
        "            or is temporarily unavailable. Please check the URL or return to the homepage.\n"
        "        </p>\n"
        "        <a href=\"/\" class=\"home-button\">Go to Homepage</a>\n"
        "    </div>\n"
        "</body>\n"
        "</html>";
    send_header(client, 404, "Not Found", "text/html", (long)strlen(body), 0);
    send(client, body, (int)strlen(body), 0);
}

void send_redirect(SOCKET client, const char* location) {
    char header[BUFFER_SIZE];
    int header_len = sprintf(header,
        "HTTP/1.1 301 Moved Permanently\r\n"
        "Location: %s\r\n"
        "Connection: close\r\n"
        "Content-Length: 0\r\n"
        "\r\n",
        location);
    send(client, header, header_len, 0);
}