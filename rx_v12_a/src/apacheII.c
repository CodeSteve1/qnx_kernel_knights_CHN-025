/*
 * apacheii.c
 *
 *  Created on: Sep 18, 2026
 *      Author: sen
 */

#include <PatientParameters.h>

int get_temp_points(float temp_c) {
    if (temp_c >= 41.0 || temp_c <= 29.9) return 4;
    if ((temp_c >= 39.0 && temp_c <= 40.9) || (temp_c >= 30.0 && temp_c <= 31.9)) return 3;
    if (temp_c >= 32.0 && temp_c <= 33.9) return 2;
    if ((temp_c >= 38.5 && temp_c <= 38.9) || (temp_c >= 34.0 && temp_c <= 35.9)) return 1;
    return 0;
}

int get_map_points(float map_val) {
    if (map_val >= 160 || map_val <= 49) return 4;
    if (map_val >= 130 && map_val <= 159) return 3;
    if ((map_val >= 110 && map_val <= 129) || (map_val >= 50 && map_val <= 69)) return 2;
    return 0;
}

int get_hr_points(int hr) {
    if (hr >= 180 || hr <= 39) return 4;
    if ((hr >= 140 && hr <= 179) || (hr >= 40 && hr <= 54)) return 3;
    if ((hr >= 110 && hr <= 139) || (hr >= 55 && hr <= 69)) return 2;
    return 0;
}

int get_rr_points(int rr) {
    if (rr >= 50 || rr <= 5) return 4;
    if (rr >= 35 && rr <= 49) return 3;
    if (rr >= 6 && rr <= 9) return 2;
    if ((rr >= 25 && rr <= 34) || (rr >= 10 && rr <= 11)) return 1;
    return 0;
}

int get_ph_points(float ph) {
    if (ph >= 7.7f || ph < 7.15f) return 4;
    if ((ph >= 7.6f && ph < 7.7f) || (ph >= 7.15f && ph < 7.25f)) return 3;
    if (ph >= 7.25f && ph < 7.33f) return 2;
    if (ph >= 7.5f && ph < 7.6f) return 1;
    return 0;
}

int get_sodium_points(float na) {
    if (na >= 180 || na <= 110) return 4;
    if ((na >= 160 && na <= 179) || (na >= 111 && na <= 119)) return 3;
    if ((na >= 155 && na <= 159) || (na >= 120 && na <= 129)) return 2;
    if (na >= 150 && na <= 154) return 1;
    return 0;
}

int get_potassium_points(float k) {
    if (k >= 7.0 || k < 2.5) return 4;
    if (k >= 6.0 && k <= 6.9) return 3;
    if (k >= 2.5 && k <= 2.9) return 2;
    if ((k >= 5.5 && k <= 5.9) || (k >= 3.0 && k <= 3.4)) return 1;
    return 0;
}

int get_creatinine_points(float cr, int acute_renal_failure) {
    int pts = 0;
    if (cr >= 3.5) pts = 4;
    else if (cr >= 2.0 && cr <= 3.4) pts = 3;
    else if (cr >= 1.5 && cr <= 1.9) pts = 2;
    else if (cr < 0.6) pts = 2;

    if (acute_renal_failure) pts *= 2;
    return pts;
}

int get_hct_points(float hct) {
    if (hct >= 60.0 || hct < 20.0) return 4;
    if ((hct >= 50.0 && hct <= 59.9) || (hct >= 20.0 && hct <= 29.9)) return 2;
    if (hct >= 46.0 && hct <= 49.9) return 1;
    return 0;
}

int get_wbc_points(float wbc) {
    if (wbc >= 40.0 || wbc < 1.0) return 4;
    if ((wbc >= 20.0 && wbc <= 39.9) || (wbc >= 1.0 && wbc <= 2.9)) return 2;
    if (wbc >= 15.0 && wbc <= 19.9) return 1;
    return 0;
}

int get_oxygenation_points(float fio2, float pao2, float a_a_gradient) {
    if (fio2 >= 0.5) {
        if (a_a_gradient >= 500) return 4;
        if (a_a_gradient >= 350 && a_a_gradient <= 499) return 3;
        if (a_a_gradient >= 200 && a_a_gradient <= 349) return 2;
        return 0;
    } else {
        if (pao2 < 55) return 4;
        if (pao2 >= 55 && pao2 <= 60) return 3;
        if (pao2 >= 61 && pao2 <= 70) return 1;
        return 0;
    }
}

int get_age_points(int age) {
    if (age >= 75) return 6;
    if (age >= 65) return 5;
    if (age >= 55) return 3;
    if (age >= 45) return 2;
    return 0;
}

int calculate_total_apache_ii(RawData raw_data) {
    int aps = (
        get_temp_points(raw_data.temp_c) +
        get_map_points(raw_data.map_val) +
        get_hr_points(raw_data.hr) +
        get_rr_points(raw_data.rr) +
        get_oxygenation_points(raw_data.fio2, raw_data.pao2, raw_data.a_a_gradient) +
        get_ph_points(raw_data.ph) +
        get_sodium_points(raw_data.na) +
        get_potassium_points(raw_data.k) +
        get_creatinine_points(raw_data.cr, raw_data.acute_renal_failure) +
        get_hct_points(raw_data.hct) +
        get_wbc_points(raw_data.wbc) +
        (15 - raw_data.gcs)
    );

    int total_score = aps + get_age_points(raw_data.age) + raw_data.chronic_health_points;
    return total_score;
}
