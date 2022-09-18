#ifndef __JSON_H
#define __JSON_H

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

#pragma pack(1)

struct json_encode_op;
struct json_decode_op;

typedef struct {
    char *buf;
    char *end;
    const struct json_encode_op *encode;
    const struct json_decode_op *decode;
} json_t;

typedef struct json_encode_op {
    void (* set_long) (json_t *json, const char *key, long value);
    void (* set_bool) (json_t *json, const char *key, bool value);
    void (* set_str) (json_t *json, const char *key, const char *value);
    void (* delete_key) (json_t *json, const char *key);
    void (* append_list_long) (json_t *json, const char *key, long value);
    void (* append_list_bool) (json_t *json, const char *key, bool value);
    void (* append_list_str) (json_t *json, const char *key, const char *value);
} json_encode_op_t;

typedef struct json_decode_op {
    long (* get_long) (json_t *json, const char *key, long deft);
    bool (* get_bool) (json_t *json, const char *key, bool deft);
    const char* (* get_str) (json_t *json, const char *key);
    bool (* is_null) (json_t *json, const char *key);
    size_t (* get_list_count) (json_t *json, const char *key);
    size_t (* get_list_long) (json_t *json, const char *key, long *buf, size_t n);
    size_t (* get_list_byte) (json_t *json, const char *key, uint8_t *buf, size_t n);
    size_t (* get_list_str) (json_t *json, const char *key, const char **buf, size_t n);
    long (* get_list_long_of) (json_t *json, const char *key, size_t index);
    const char* (* get_list_str_of) (json_t *json, const char *key, size_t index);
} json_decode_op_t;

#pragma pack()

void json_init(json_t *json, char *buf, size_t size, bool clearbuf);

#endif
