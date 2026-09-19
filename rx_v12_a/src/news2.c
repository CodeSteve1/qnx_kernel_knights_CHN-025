#include "PatientParameters.h"

int get_news2_rr(int rr) {
    if (rr <= 8 || rr >= 25) return 3;
    if (rr >= 21 && rr <= 24) return 2;
    if (rr >= 9 && rr <= 11) return 1;
    return 0;
}

int get_news2_spo2(int spo2) {
    if (spo2 <= 91) return 3;
    if (spo2 == 92 || spo2 == 93) return 2;
    if (spo2 == 94 || spo2 == 95) return 1;
    return 0;
}

int get_news2_oxygen(int on_oxygen) {
    if (on_oxygen) return 2;
    return 0;
}

int get_news2_sbp(int sbp) {
    if (sbp <= 90 || sbp >= 220) return 3;
    if (sbp >= 91 && sbp <= 100) return 2;
    if (sbp >= 101 && sbp <= 110) return 1;
    return 0;
}

int get_news2_hr(int hr) {
    if (hr <= 40 || hr >= 131) return 3;
    if (hr >= 111 && hr <= 130) return 2;
    if ((hr >= 41 && hr <= 50) || (hr >= 91 && hr <= 110)) return 1;
    return 0;
}

int get_news2_consciousness(int is_alert) {
    if (!is_alert) return 3;
    return 0;
}

int get_news2_temp(float temp_c) {
    if (temp_c <= 35.0) return 3;
    if (temp_c >= 39.1) return 2;
    if ((temp_c >= 35.1 && temp_c <= 36.0) || (temp_c >= 38.1 && temp_c <= 39.0)) return 1;
    return 0;
}

int calculate_total_news2(RawData data) {
    return get_news2_rr(data.rr) +
           get_news2_spo2(data.spo2) +
           get_news2_oxygen(data.on_oxygen) +
           get_news2_sbp(data.sbp) +
           get_news2_hr(data.hr) +
           get_news2_consciousness(data.is_alert) +
           get_news2_temp(data.temp_c);
}
