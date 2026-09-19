#include "acquisition.h"
#include "patient_data.h"
#include "config.h"
#include "scheduling.h"
#include "rtci.h"
#include "time_utils.h"
#include "history_client.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <pthread.h>
#include <poll.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/mman.h>
#include <sys/stat.h>

/* --- QNX Specific Headers (pulse notification to the display process) --- */
#include <sys/neutrino.h>
#include <sys/dispatch.h>

static int g_lock_fd = -1;

/* Applies one line of TCP vitals JSON to a patient's shared-memory
 * record, and, if the patient is flagged critical, sends a QNX pulse
 * to the display process carrying its RTCI severity as the pulse
 * priority (so the display can react immediately rather than waiting
 * for the next poll). */
static void apply_tcp_line(int p_idx, char *line, long long recv_ts) {
    char *hr_ptr = strstr(line, "\"heart_rate\":");
    char *spo2_ptr = strstr(line, "\"spo2\":");
    char *rr_ptr = strstr(line, "\"rr\":");
    char *bp_ptr = strstr(line, "\"blood_pressure\":");
    char *ts_ptr = strstr(line, "\"timestamp\":");
    char *crit_ptr = strstr(line, "\"critical\":");

    if (ts_ptr) g_shm->patients[p_idx].last_latency = recv_ts - atoll(ts_ptr + 12);
    if (hr_ptr) g_shm->patients[p_idx].hr = atoi(hr_ptr + 13);
    if (spo2_ptr) g_shm->patients[p_idx].spo2 = atoi(spo2_ptr + 7);
    if (rr_ptr) g_shm->patients[p_idx].rr = atoi(rr_ptr + 5);

    if (crit_ptr) g_shm->patients[p_idx].critical = (strstr(crit_ptr, "true") != NULL && (strstr(crit_ptr, "true") - crit_ptr) < 15) ? 1 : 0;
    if (bp_ptr) {
        char *start = strchr(bp_ptr + 17, '"');
        if (start) {
            start++; char *end = strchr(start, '"');
            if (end && (end - start) < 15) {
                strncpy(g_shm->patients[p_idx].bp, start, end - start);
                g_shm->patients[p_idx].bp[end - start] = '\0';
            }
        }
    }

    if (g_shm->patients[p_idx].critical) {
        if (g_pulse_coid == -1) g_pulse_coid = name_open(ATTACH_NAME, 0);
        if (g_pulse_coid != -1) {
            float severity = calculate_rtci(p_idx);
            MsgSendPulse(g_pulse_coid, (int)(severity * 100), _PULSE_CODE_MINAVAIL, p_idx);
        }
    }
}

typedef struct {
    int p_idx;
    int listen_fd;
    int client_fd;
    int listen_pfd_idx;
    int client_pfd_idx;
    char readbuf[512];
    char linebuf[1024];
    int line_len;
} TCPPatientState;

/* Owns PATIENTS_PER_TCP_THREAD listening sockets (one per patient in
 * its slice), accepts a single client per patient, and turns
 * newline-delimited JSON lines into apply_tcp_line() calls. */
static void* PatientTCPPoolWorker(void* arg) {
    elevate_io_thread();
    int pool_idx = (int)(intptr_t)arg;
    int start_p_idx = pool_idx * PATIENTS_PER_TCP_THREAD;

    TCPPatientState state[PATIENTS_PER_TCP_THREAD];
    struct pollfd pfds[PATIENTS_PER_TCP_THREAD * 2];

    for (int i = 0; i < PATIENTS_PER_TCP_THREAD; i++) {
        int p_idx = start_p_idx + i;
        state[i].p_idx = p_idx;
        state[i].client_fd = -1;
        state[i].line_len = 0;

        int port = TCP_START_PORT + p_idx + 1;
        state[i].listen_fd = socket(AF_INET, SOCK_STREAM, 0);
        int opt = 1;
        setsockopt(state[i].listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

        struct sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = htons(port);

        if (bind(state[i].listen_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
            fprintf(stderr, "Patient %d TCP bind failed\n", p_idx + 1);
        }
        listen(state[i].listen_fd, 5);
        set_nonblocking(state[i].listen_fd);
    }

    while (1) {
        int nfds = 0;

        for (int i = 0; i < PATIENTS_PER_TCP_THREAD; i++) {
            state[i].listen_pfd_idx = nfds;
            pfds[nfds].fd = state[i].listen_fd;
            pfds[nfds].events = POLLIN;
            nfds++;

            if (state[i].client_fd >= 0) {
                state[i].client_pfd_idx = nfds;
                pfds[nfds].fd = state[i].client_fd;
                pfds[nfds].events = POLLIN;
                nfds++;
            } else {
                state[i].client_pfd_idx = -1;
            }
        }

        int ret = poll(pfds, nfds, -1);
        if (ret > 0) {
            for (int i = 0; i < PATIENTS_PER_TCP_THREAD; i++) {
                int l_idx = state[i].listen_pfd_idx;
                if (pfds[l_idx].revents & POLLIN) {
                    int new_fd = accept(state[i].listen_fd, NULL, NULL);
                    if (new_fd >= 0) {
                        if (state[i].client_fd >= 0) close(state[i].client_fd);
                        set_nonblocking(new_fd);
                        state[i].client_fd = new_fd;
                        state[i].line_len = 0;
                    }
                }

                int c_idx = state[i].client_pfd_idx;
                if (c_idx >= 0) {
                    if (pfds[c_idx].revents & (POLLIN | POLLERR | POLLHUP)) {
                        int valread = read(state[i].client_fd, state[i].readbuf, sizeof(state[i].readbuf));
                        if (valread <= 0) {
                            close(state[i].client_fd);
                            state[i].client_fd = -1;
                        } else {
                            long long recv_ts = get_current_time_ms();
                            for (int k = 0; k < valread; k++) {
                                if (state[i].readbuf[k] == '\n') {
                                    state[i].linebuf[state[i].line_len] = '\0';
                                    apply_tcp_line(state[i].p_idx, state[i].linebuf, recv_ts);
                                    state[i].line_len = 0;
                                } else if (state[i].line_len < 1023) {
                                    state[i].linebuf[state[i].line_len++] = state[i].readbuf[k];
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    return NULL;
}

/* Owns one patient's UDP socket, used only for the high-rate ECG
 * waveform samples (kept off the TCP vitals path so a slow vitals
 * connection can never stall the waveform). */
static void* PatientUDPWorker(void* arg) {
    elevate_io_thread();
    int p_idx = (int)(intptr_t)arg;
    int port = UDP_START_PORT + p_idx + 1;

    int fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    int opt = 1, rcvbuf = 1024 * 1024;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &rcvbuf, sizeof(rcvbuf));

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    bind(fd, (struct sockaddr *)&addr, sizeof(addr));
    set_nonblocking(fd);

    struct pollfd pfd = { fd, POLLIN, 0 };
    char buffer[512];

    while (1) {
        poll(&pfd, 1, -1);
        if (pfd.revents & POLLIN) {
            int max_drains = 100;
            while(max_drains--) {
                int recv_len = recvfrom(fd, buffer, sizeof(buffer)-1, 0, NULL, NULL);
                if (recv_len <= 0) break;
                buffer[recv_len] = '\0';

                char *ecg_ptr = strstr(buffer, "\"ecg\":");
                if (ecg_ptr) {
                    float ecg_val = atof(ecg_ptr + 6);
                    if (ecg_val != ecg_val) ecg_val = 0.0f;
                    else if (ecg_val > 5.0f) ecg_val = 5.0f;
                    else if (ecg_val < -5.0f) ecg_val = -5.0f;

                    int idx = g_shm->patients[p_idx].history_idx;
                    g_shm->patients[p_idx].ecg_history[idx] = ecg_val;
                    g_shm->patients[p_idx].history_idx = (idx + 1) % GRAPH_PTS;
                }
            }
        }
    }
    return NULL;
}

int run_acquisition(void) {
    printf("Starting Acquisition Process (Network & Shared Memory)...\n");

    g_lock_fd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(LOCK_PORT);
    if (bind(g_lock_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        fprintf(stderr, "FATAL: Another acquisition instance is already running.\n"); exit(1);
    }
    listen(g_lock_fd, 1);

    int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) { perror("shm_open failed"); exit(1); }
    ftruncate(shm_fd, sizeof(SharedSystemData));

    g_shm = mmap(NULL, sizeof(SharedSystemData), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (g_shm == MAP_FAILED) { perror("mmap failed"); exit(1); }

    for (int i = 0; i < NUM_PATIENTS; i++) {
        g_shm->patients[i].id = i + 1;
        g_shm->patients[i].hr = 70;
        g_shm->patients[i].spo2 = 98;
        g_shm->patients[i].rr = 16;
        g_shm->patients[i].critical = 0;
        g_shm->patients[i].last_latency = 0; // Initialize latency here
        strcpy(g_shm->patients[i].bp, "120/80");
        for(int j=0; j<GRAPH_PTS; j++) g_shm->patients[i].ecg_history[j] = 0.0f;

        fetch_initial_clinical_data(i);
    }

    for (int i = 0; i < TCP_POOL_SIZE; i++) {
        pthread_t t1;
        pthread_create(&t1, NULL, PatientTCPPoolWorker, (void*)(intptr_t)i);
    }

    for (int i = 0; i < NUM_PATIENTS; i++) {
        pthread_t t2;
        pthread_create(&t2, NULL, PatientUDPWorker, (void*)(intptr_t)i);
    }

    while(1) sleep(10);
    return 0;
}
