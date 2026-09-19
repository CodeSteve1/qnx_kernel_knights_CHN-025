#include <stdio.h>
#include "PatientParameters.h"

extern int parse_data(const char* json_string, RawData* patient);
extern int calculate_total_apache_ii(RawData raw_data);
extern int calculate_total_news2(RawData data);

float get_final_rtci_from_json(const char* json_string) {
    RawData patient = {0};
    
    if (parse_data(json_string, &patient) != 0) {
        printf("JSON Parsing Failed\n");
        return -1.0f;
    }

    int news2 = calculate_total_news2(patient);
    int apache2 = calculate_total_apache_ii(patient);

    return (float)news2 + ((float)apache2 * 0.15f);
}

