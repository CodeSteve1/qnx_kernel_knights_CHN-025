#include "history_client.h"
#include "json_utils.h"
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

/* Implemented by the external RTCI engine (linked separately). */
extern int parse_data(const char* json_string, RawData* patient);

void* fetch_worker(void* arg) {
    int p_id = (int)(intptr_t)arg;
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) return NULL;

    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons(HISTORY_SERVER_PORT);
    inet_pton(AF_INET, HISTORY_SERVER_IP, &server.sin_addr);

    struct timeval tv; tv.tv_sec = 2; tv.tv_usec = 0;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (const char*)&tv, sizeof tv);

    if (connect(sock, (struct sockaddr*)&server, sizeof(server)) < 0) {
        sprintf(g_modal_text, "ERROR:\nUNABLE TO CONNECT TO PI 2\nAT %s:%d", HISTORY_SERVER_IP, HISTORY_SERVER_PORT);
        g_show_modal_for_patient = p_id;
        close(sock);
        return NULL;
    }

    char req[256];
    sprintf(req, "GET /patient/%d HTTP/1.1\r\nHost: %s\r\nConnection: close\r\n\r\n", p_id, HISTORY_SERVER_IP);
    send(sock, req, strlen(req), 0);

    char resp_buf[8192];
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
        char name[64], diag[64], btype[16], date[32], allergies[128], history[256], notes[256];
        extract_json_str(body, "name", name, sizeof(name));
        extract_json_str(body, "primary_diagnosis", diag, sizeof(diag));
        extract_json_str(body, "blood_type", btype, sizeof(btype));
        extract_json_str(body, "admission_date", date, sizeof(date));
        extract_json_arr(body, "allergies", allergies, sizeof(allergies));
        extract_json_arr(body, "medical_history", history, sizeof(history));
        extract_json_str(body, "doctor_notes", notes, sizeof(notes));

        snprintf(g_modal_text, sizeof(g_modal_text),
            "- NAME: %s\n"
            "- ADMISSION: %s\n"
            "- DIAGNOSIS: %s\n"
            "- BLOOD TYPE: %s\n"
            "- ALLERGIES: %s\n"
            "- HISTORY: %s\n"
            "- DOCTOR NOTES: %s\n",
            name, date, diag, btype, allergies, history, notes);
    }

    // Reset AI state when opening a new modal
    g_ai_state = 0;
    memset(g_ai_summary_text, 0, sizeof(g_ai_summary_text));
    g_show_modal_for_patient = p_id;
    return NULL;
}

void fetch_initial_clinical_data(int p_idx) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) return;

    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons(HISTORY_SERVER_PORT);
    inet_pton(AF_INET, HISTORY_SERVER_IP, &server.sin_addr);

    if (connect(sock, (struct sockaddr*)&server, sizeof(server)) < 0) {
        close(sock); return;
    }

    char req[256];
    int p_id = g_shm->patients[p_idx].id;
    sprintf(req, "GET /patient/%d HTTP/1.1\r\nHost: %s\r\nConnection: close\r\n\r\n", p_id, HISTORY_SERVER_IP);
    send(sock, req, strlen(req), 0);

    char resp_buf[8192];
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
        parse_data(body, &g_shm->patients[p_idx].clinical_data);
    }
}
