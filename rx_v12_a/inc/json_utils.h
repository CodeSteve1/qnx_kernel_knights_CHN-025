#ifndef JSON_UTILS_H
#define JSON_UTILS_H

/* Minimal hand-rolled JSON scrapers used to pull a handful of known
 * fields out of the small HTTP responses returned by the history
 * server (Pi 2), without pulling in a full JSON parser. */

/* Extracts a string value for "key": "..." into out (NUL-terminated,
 * truncated to max_len). Writes "N/A" if the key isn't found or the
 * value doesn't fit. */
void extract_json_str(const char* json, const char* key, char* out, int max_len);

/* Extracts the raw contents of a "key": [ ... ] array (quotes and
 * newlines replaced with spaces) into out. Writes "None" if the key
 * isn't found or the value doesn't fit. */
void extract_json_arr(const char* json, const char* key, char* out, int max_len);

#endif /* JSON_UTILS_H */
