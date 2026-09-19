#ifndef APACHEII
#define APACHEII

typedef struct {
    float temp_c;
    float map_val;
    int hr;
    int spo2;
    int on_oxygen;
    int rr;
    float fio2;
    float pao2;
    float a_a_gradient;
    float ph;
    float na;
    float k;
    float cr;
    int acute_renal_failure;
    float hct;
    float wbc;
    int gcs;
    int age;
    int sbp;
    int chronic_health_points;
    int is_alert;
} RawData;
 
#endif
