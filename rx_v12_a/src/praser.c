#include "cJSON.h"
#include "PatientParameters.h"

int parse_data(const char* json_string, RawData* patient) {
    cJSON *root = cJSON_Parse(json_string);
    if (!root) return -1;

    cJSON *item;

    item = cJSON_GetObjectItem(root, "Temperature_in_Celsius_[temp_c]");
    if (item) patient->temp_c = (float)item->valuedouble;

    item = cJSON_GetObjectItem(root, "Mean_Arterial_Pressure_[map_val]");
    if (item) patient->map_val = (float)item->valuedouble;

    item = cJSON_GetObjectItem(root, "Heart_Rate_[hr]");
    if (item) patient->hr = item->valueint;

    item = cJSON_GetObjectItem(root, "Oxygen_Saturation_[spo2]");
    if (item) patient->spo2 = item->valueint;

    item = cJSON_GetObjectItem(root, "On_Supplemental_Oxygen_[on_oxygen]");
    if (item) patient->on_oxygen = item->valueint;

    item = cJSON_GetObjectItem(root, "Respiratory_Rate_[rr]");
    if (item) patient->rr = item->valueint;

    item = cJSON_GetObjectItem(root, "Fraction_of_Inspired_Oxygen_[fio2]");
    if (item) patient->fio2 = (float)item->valuedouble;

    item = cJSON_GetObjectItem(root, "Partial_Pressure_of_Arterial_Oxygen_[pao2]");
    if (item) patient->pao2 = (float)item->valuedouble;

    item = cJSON_GetObjectItem(root, "Alveolar-Arterial_Oxygen_Gradient_[a_a_gradient]");
    if (item) patient->a_a_gradient = (float)item->valuedouble;

    item = cJSON_GetObjectItem(root, "Arterial_pH_[ph]");
    if (item) patient->ph = (float)item->valuedouble;

    item = cJSON_GetObjectItem(root, "Serum_Sodium_[na]");
    if (item) patient->na = (float)item->valuedouble;

    item = cJSON_GetObjectItem(root, "Serum_Potassium_[k]");
    if (item) patient->k = (float)item->valuedouble;

    item = cJSON_GetObjectItem(root, "Serum_Creatinine_[cr]");
    if (item) patient->cr = (float)item->valuedouble;

    item = cJSON_GetObjectItem(root, "Acute_Renal_Failure_Presence_[acute_renal_failure]");
    if (item) patient->acute_renal_failure = item->valueint;

    item = cJSON_GetObjectItem(root, "Hematocrit_[hct]");
    if (item) patient->hct = (float)item->valuedouble;

    item = cJSON_GetObjectItem(root, "White_Blood_Cell_Count_[wbc]");
    if (item) patient->wbc = (float)item->valuedouble;

    item = cJSON_GetObjectItem(root, "Glasgow_Coma_Scale_[gcs]");
    if (item) patient->gcs = item->valueint;

    item = cJSON_GetObjectItem(root, "Age_[age]");
    if (item) patient->age = item->valueint;

    item = cJSON_GetObjectItem(root, "Systolic_Blood_Pressure_[sbp]");
    if (item) patient->sbp = item->valueint;

    item = cJSON_GetObjectItem(root, "Chronic_Health_Points_[chronic_health_points]");
    if (item) patient->chronic_health_points = item->valueint;

    item = cJSON_GetObjectItem(root, "Level_of_Consciousness_Alert_[is_alert]");
    if (item) patient->is_alert = item->valueint;

    cJSON_Delete(root);
    return 0;
}
