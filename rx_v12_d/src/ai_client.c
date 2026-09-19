#include "ai_client.h"
#include "patient_data.h"
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>

void* ai_network_worker(void* arg) {
    int p_id = (int)(intptr_t)arg;
    g_ai_state = 1;
    memset(g_ai_summary_text, 0, sizeof(g_ai_summary_text));

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        strcpy(g_ai_summary_text, "ERROR: FAILED TO OPEN SOCKET");
        g_ai_state = 2;
        return NULL;
    }

    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons(HISTORY_SERVER_PORT);
    inet_pton(AF_INET, HISTORY_SERVER_IP, &server.sin_addr);

    struct timeval tv; tv.tv_sec = 120; tv.tv_usec = 0; // Model inference can take time
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (const char*)&tv, sizeof tv);

    if (connect(sock, (struct sockaddr*)&server, sizeof(server)) < 0) {
        snprintf(g_ai_summary_text, sizeof(g_ai_summary_text),
                 "ERROR: UNABLE TO CONNECT TO AI SERVER AT %s:%d",
                 HISTORY_SERVER_IP, HISTORY_SERVER_PORT);
        g_ai_state = 2;
        close(sock);
        return NULL;
    }

    char req[256];
    snprintf(req, sizeof(req), "GET /patient/%d/ai_summary HTTP/1.1\r\nHost: %s\r\nConnection: close\r\n\r\n", p_id, HISTORY_SERVER_IP);
    send(sock, req, strlen(req), 0);

    char resp_buf[4096];
    int total = 0;
    while(total < (int)sizeof(resp_buf) - 1) {
        int b = recv(sock, resp_buf + total, sizeof(resp_buf) - 1 - total, 0);
        if (b <= 0) break;
        total += b;
    }
    resp_buf[total] = '\0';
    close(sock);

    char *body = strstr(resp_buf, "\r\n\r\n");
    if (body) {
        body += 4;
        strncpy(g_ai_summary_text, body, sizeof(g_ai_summary_text) - 1);
    } else {
        strncpy(g_ai_summary_text, resp_buf, sizeof(g_ai_summary_text) - 1);
    }

    g_ai_state = 2;
    return NULL;
}
