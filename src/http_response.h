#ifndef HTTP_RESPONSE_H
#define HTTP_RESPONSE_H

#include "server.h"

void send_header(SOCKET client, int status_code, const char* status_text, 
                 const char* mime_type, long content_length, time_t modified_time);
void send_404(SOCKET client);
void send_redirect(SOCKET client, const char* location);

#endif