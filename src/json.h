#ifndef __JSON_H__
#define __JSON_H__

#include <stdint.h>
#include <stdlib.h>

#define CONFIG_JSON_KV_MAX_COUNT   32

#ifndef TRUE
#define TRUE    1
#endif

#ifndef FALSE
#define FALSE    0
#endif

#ifndef MIN
    #define MIN(a, b)   ((a) < (b) ? (a) : (b))
#endif

#ifndef START_WITH
    #define START_WITH(str, dest)   (0 == strncmp(str, dest, MIN(strlen(str), strlen(dest))))
#endif

#ifndef SUCCESS
#define SUCCESS                         0
#endif
#define CONFIG_JSON_ERR_START           -1
#define ERR_JSON_FORMAT                 (CONFIG_JSON_ERR_START)
#define ERR_JSON_KEY_NOT_FOUND          (CONFIG_JSON_ERR_START - 1)
#define ERR_JSON_VALUE_TOO_LONG         (CONFIG_JSON_ERR_START - 2)
#define ERR_JSON_KEY_TOO_LONG           (CONFIG_JSON_ERR_START - 3)

typedef struct {
    #define JSON_TYPE_BITS       3
    uint32_t type: JSON_TYPE_BITS,
                keyhash: 8 * sizeof(uint32_t) - JSON_TYPE_BITS;

        #define CUT_KEYHASH(hash)       (hash)
        #define JSON_TYPE_INT       0
        #define JSON_TYPE_STR       1
        #define JSON_TYPE_BOOL      2
        #define JSON_TYPE_NULL      3
        #define JSON_TYPE_LIST      4

    union {
        int vint;
        int vbool;
        int vnull;
        unsigned int vlist_count;
        char *vstr;
    } value;
} json_value_t;

struct json_decode_op;

typedef struct {
    uint32_t count;
    json_value_t values[CONFIG_JSON_KV_MAX_COUNT];
    const struct json_decode_op *op;
} json_decode_t;

typedef struct json_decode_op {
    int (* get_int) (json_decode_t *json, const char *key, int def);
    int (* get_bool) (json_decode_t *json, const char *key, int def);
    const char* (* get_str) (json_decode_t *json, const char *key);
    int (* is_null) (json_decode_t *json, const char *key);
    unsigned int (* get_list_count) (json_decode_t *json, const char *key);
} json_decode_op_t;

struct json_encode_op;

typedef struct {
    char *jsonstr;
    size_t size;
    const struct json_encode_op *op;
} json_encode_t;

typedef struct json_encode_op {
    void (* set_int) (json_encode_t *json, const char *key, int value);
    void (* set_bool) (json_encode_t *json, const char *key, int value);
    void (* set_str) (json_encode_t *json, const char *key, const char *value);
    void (* delete_key) (json_encode_t *json, const char *key);
    void (* append_list_int) (json_encode_t *json, const char *key, int value);
    void (* append_list_bool) (json_encode_t *json, const char *key, int value);
    void (* append_list_str) (json_encode_t *json, const char *key, const char *value);
} json_encode_op_t;

void json_encode_init(json_encode_t *json, char *buf, size_t size);

#endif
