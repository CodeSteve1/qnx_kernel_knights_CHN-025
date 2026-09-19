#include "json_utils.h"

#include <stdio.h>
#include <string.h>

void extract_json_str(const char* json, const char* key, char* out, int max_len) {
    char search[128];
    snprintf(search, sizeof(search), "\"%s\":", key);
    const char* p = strstr(json, search);
    if (p) {
        p += strlen(search);
        while (*p == ' ' || *p == '\t') p++;
        if (*p == '"') {
            p++;
            const char* end = strchr(p, '"');
            if (end && (end - p) < max_len) {
                strncpy(out, p, end - p);
                out[end - p] = '\0';
                return;
            }
        }
    }
    strcpy(out, "N/A");
}

void extract_json_arr(const char* json, const char* key, char* out, int max_len) {
    char search[128];
    snprintf(search, sizeof(search), "\"%s\":", key);
    const char* p = strstr(json, search);
    if (p) {
        p += strlen(search);
        while (*p == ' ' || *p == '\t') p++;
        if (*p == '[') {
            p++;
            const char* end = strchr(p, ']');
            if (end && (end - p) < max_len) {
                strncpy(out, p, end - p);
                out[end - p] = '\0';
                for(int i = 0; out[i]; i++) {
                    if(out[i] == '"' || out[i] == '\n' || out[i] == '\r') out[i] = ' ';
                }
                return;
            }
        }
    }
    strcpy(out, "None");
}
